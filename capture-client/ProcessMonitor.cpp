#include "ProcessMonitor.h"
#include "EventController.h"
#include "ProcessManager.h"
#include <boost/bind.hpp>

#define IOCTL_CAPTURE_GET_PROCINFO	CTL_CODE(0x00000022, 0x0802, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_CAPTURE_PROC_LIST    CTL_CODE(0x00000022, 0x0807, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)

typedef struct _ProcessInfo
{
	TIME_FIELDS time;
    DWORD  ParentId;
    DWORD  ProcessId;
    BOOLEAN bCreate;
	UINT processPathLength;
	WCHAR processPath[1024];
} ProcessInfo;

typedef struct _PROCESS_TUPLE
{
	DWORD processID;
	WCHAR name[1024];
} PROCESS_TUPLE, * PPROCESS_TUPLE;

ProcessMonitor::ProcessMonitor(void)
{
	wchar_t kernelDriverPath[1024];
	wchar_t exListDriverPath[1024];

	hMonitorStoppedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	driverInstalled = false;
	monitorRunning = false;
	hEvent = INVALID_HANDLE_VALUE;
	
	// Load exclusion list
	GetFullPathName(L"ProcessMonitor.exl", 1024, exListDriverPath, NULL);
	Monitor::loadExclusionList(exListDriverPath);

	onProcessExclusionReceivedConnection = EventController::getInstance()->connect_onServerEvent(L"process-exclusion", boost::bind(&ProcessMonitor::onProcessExclusionReceived, this, _1));

	// Load process monitor kernel driver
	GetFullPathName(L"CaptureProcessMonitor.sys", 1024, kernelDriverPath, NULL);
	if(Monitor::installKernelDriver(kernelDriverPath, L"CaptureProcessMonitor", L"Capture Process Monitor"))
	{	
		hDriver = CreateFile(
					L"\\\\.\\CaptureProcessMonitor",
					GENERIC_READ | GENERIC_WRITE, 
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					0,                     // Default security
					OPEN_EXISTING,
					FILE_FLAG_OVERLAPPED,  // Perform asynchronous I/O
					0);                    // No template
		if(INVALID_HANDLE_VALUE == hDriver) {
			printf("ProcessMonitor: ERROR - CreateFile Failed: %08x\n", GetLastError());
		} else {
			initialiseKernelDriverProcessMap();
			driverInstalled = true;
		}
	}
}

ProcessMonitor::~ProcessMonitor(void)
{
	processManagerConnection.disconnect();
	stop();
	if(driverInstalled)
	{
		driverInstalled = false;
		CloseHandle(hDriver);	
	}
	CloseHandle(hMonitorStoppedEvent);
}

boost::signals::connection 
ProcessMonitor::connect_onProcessEvent(const signal_processEvent::slot_type& s)
{ 
	DebugPrintTrace(L"ProcessMonitor::connect_onProcessEvent start\n");
	DebugPrintTrace(L"ProcessMonitor::connect_onProcessEvent end\n");
	return signalProcessEvent.connect(s); 
}

void
ProcessMonitor::onProcessExclusionReceived(const Element& element)
{
	DebugPrintTrace(L"ProcessMonitor::onProcessExclusionReceived start\n");
	std::wstring excluded = L"";
	std::wstring parentProcessPath = L"";
	std::wstring processPath = L"";
	std::wstring action = L"";

	std::vector<Attribute>::const_iterator it;
	for(it = element.getAttributes().begin(); it != element.getAttributes().end(); it++)
	{
		if((*it).getName() == L"subject") {
			parentProcessPath = (*it).getValue();
		} else if((*it).getName() == L"object") {
			processPath = (*it).getValue();
		} else if((*it).getName() == L"excluded") {
			excluded = (*it).getValue();
		} else if((*it).getName() == L"action") {
			action = (*it).getValue();
		}
	}


	Monitor::addExclusion(excluded, action, parentProcessPath, processPath);
	DebugPrintTrace(L"ProcessMonitor::onProcessExclusionReceived end\n");
}

void
ProcessMonitor::initialiseKernelDriverProcessMap()
{
	DebugPrintTrace(L"ProcessMonitor::initialiseKernelDriverProcessMap start\n");
	ProcessManager* processManager = ProcessManager::getInstance();
	stdext::hash_map<DWORD, std::wstring> processMap;

	processMap = processManager->getProcessMap();
	stdext::hash_map<DWORD, std::wstring>::iterator it;
	for(it = processMap.begin(); it != processMap.end(); it++)
	{
		DWORD dwReturn;
		PROCESS_TUPLE pTuple;
		ZeroMemory(&pTuple, sizeof(PROCESS_TUPLE));
		pTuple.processID = it->first;
		memcpy(pTuple.name, it->second.c_str(), it->second.length()*sizeof(wchar_t));
		DeviceIoControl(hDriver,
			IOCTL_CAPTURE_PROC_LIST, 
			&pTuple, 
			sizeof(PROCESS_TUPLE), 
			0, 
			0, 
			&dwReturn, 
			NULL);
	}
	DebugPrintTrace(L"ProcessMonitor::initialiseKernelDriverProcessMap end\n");
}

void
ProcessMonitor::start()
{
	if(!isMonitorRunning() && isDriverInstalled())
	{
		hEvent = OpenEvent(SYNCHRONIZE, FALSE, L"Global\\CaptureProcDrvProcessEvent");
		processMonitorThread = new Thread(this);
		processMonitorThread->start("ProcessMonitor");
	}
}

void
ProcessMonitor::stop()
{
	if(isMonitorRunning() && isDriverInstalled())
	{
		monitorRunning = false;
		WaitForSingleObject(hMonitorStoppedEvent, 1000);
		CloseHandle(hEvent);
		DebugPrint(L"ProcessMonitor::stop() stopping thread.\n");
		processMonitorThread->stop();
		DWORD dwWaitResult;
		dwWaitResult = processMonitorThread->wait(5000);
		switch (dwWaitResult) 
		{
        // All thread objects were signaled
        case WAIT_OBJECT_0: 
            DebugPrint(L"ProcessMonitor::stop() stopped processMonitorThread.\n");
			break;
		case WAIT_TIMEOUT:
			DebugPrint(L"ProcessMonitor::stop() stopping processMonitorThread timed out. Attempting to terminate.\n");
			processMonitorThread->terminate();
			DebugPrint(L"ProcessMonitor::stop() terminated processMonitorThread.\n");
			break;
        // An error occurred
        default: 
            printf("ProcessMonitor stopping processMonitorThread failed (%d)\n", GetLastError());
		} 
		delete processMonitorThread;
	}
}

void ProcessMonitor::onProcessEvent(BOOLEAN create, DWORD processId, DWORD parentProcessId, std::wstring time) {
	DebugPrintTrace(L"ProcessMonitor::onProcessEvent() start\n");
	std::wstring processPath;
	std::wstring processModuleName;
	std::wstring parentProcessPath;
	std::wstring parentProcessModuleName;
	
	processPath = ProcessManager::getInstance()->getProcessPath(processId);
	

	DebugPrint(L"Capture-ProcessMonitor: onProcessEvent - processPath: %s\n",processPath.c_str());

	ProcessManager::getInstance()->onProcessEvent(create, time, parentProcessId, 
		processId, processPath);

	// Get process name and path
	processModuleName = ProcessManager::getInstance()->getProcessModuleName(processId);
	

	// Get parent process name and path
	parentProcessModuleName = ProcessManager::getInstance()->getProcessModuleName(parentProcessId);
	parentProcessPath = ProcessManager::getInstance()->getProcessPath(parentProcessId);
	DebugPrint(L"Capture-ProcessMonitor: onEvent %i %i:%ls -> %i:%ls\n", create, parentProcessId, parentProcessPath.c_str(), processId, processPath.c_str()); 
	if(!Monitor::isEventAllowed(processModuleName,parentProcessModuleName,processPath))
	{
		DebugPrint(L"Capture-ProcessMonitor: onEvent not allowed %i %i:%ls -> %i:%ls\n", create, parentProcessId, parentProcessPath.c_str(), processId, processPath.c_str()); 
		signalProcessEvent(create, time, parentProcessId, parentProcessPath, processId, processPath);
	}
	DebugPrintTrace(L"ProcessMonitor::onProcessEvent() end\n");
}


void
ProcessMonitor::run()
{
	DebugPrintTrace(L"ProcessMonitor::run() start\n");
	ProcessInfo tempP;
	ZeroMemory(&tempP, sizeof(tempP));
	monitorRunning = true;
	DWORD      dwBytesReturned = 0;
	while(running && isMonitorRunning())
	{

		DebugPrint(L"Capture-ProcessMonitor: waiting for hEvent signal\n");
		WaitForSingleObject(hEvent, INFINITE);
		DebugPrint(L"Capture-ProcessMonitor: hEvent signal received\n");
		BOOL       bReturnCode = FALSE;
		
		ProcessInfo p;

		bReturnCode = DeviceIoControl(
			hDriver,
			IOCTL_CAPTURE_GET_PROCINFO,
			0, 
			0,
			&p, sizeof(p),
			&dwBytesReturned,
			NULL
			);

		if(dwBytesReturned > 0)
		{
			if(p.bCreate != tempP.bCreate ||
				p.ParentId != tempP.ParentId ||
				p.ProcessId != tempP.ProcessId)
			{		
				DebugPrint(L"Capture-ProcessMonitor: onEvent1 %i %i -> %i\n", p.bCreate, p.ParentId, p.ProcessId); 

				std::wstring time = Time::timefieldToString(p.time);
				DWORD processId = p.ProcessId;
				DWORD parentProcessId = p.ParentId;
				BOOLEAN create = p.bCreate;
				
				onProcessEvent(create,processId,parentProcessId,time);

				tempP = p;
			} else {
				DebugPrint(L"Capture-ProcessMonitor: onEvent2 %i %i -> %i\n", p.bCreate, p.ParentId, p.ProcessId); 

				std::wstring processPath;
				std::wstring parentProcessPath;
				processPath = ProcessManager::getInstance()->getProcessPath(p.ProcessId);
				parentProcessPath = ProcessManager::getInstance()->getProcessPath(p.ParentId);
				
				DebugPrint(L"Capture-ProcessMonitor: onEvent already processed %i %i:%ls -> %i:%ls\n", p.bCreate, p.ParentId, parentProcessPath.c_str(), p.ProcessId, processPath.c_str()); 
			}
		}
	}
	SetEvent(hMonitorStoppedEvent);
	DebugPrintTrace(L"ProcessMonitor::run() stop\n");
}
