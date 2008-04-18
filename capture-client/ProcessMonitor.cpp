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

	//processManagerConnection = connect_onProcessEvent(boost::bind(&ProcessManager::onProcessEvent, ProcessManager::getInstance(), _1, _2, _3, _4, _5, _6));
	hMonitorStoppedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	driverInstalled = false;
	monitorRunning = false;
	hEvent = INVALID_HANDLE_VALUE;
	//v->AddVisitorListener(this);
	
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
	return signalProcessEvent.connect(s); 
}

void
ProcessMonitor::onProcessExclusionReceived(const Element& element)
{
	std::wstring excluded = L"";
	std::wstring parentProcessPath = L"";
	std::wstring processPath = L"";

	std::vector<Attribute>::const_iterator it;
	for(it = element.getAttributes().begin(); it != element.getAttributes().end(); it++)
	{
		if((*it).getName() == L"subject") {
			parentProcessPath = (*it).getValue();
		} else if((*it).getName() == L"object") {
			processPath = (*it).getValue();
		} else if((*it).getName() == L"excluded") {
			excluded = (*it).getValue();
		}
	}
	Monitor::addExclusion(excluded, L"process", parentProcessPath, processPath);
}

void
ProcessMonitor::initialiseKernelDriverProcessMap()
{
	stdext::hash_map<DWORD, std::wstring> processMap;

	processMap = ProcessManager::getInstance()->getProcessMap();
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

void
ProcessMonitor::run()
{
	ProcessInfo tempP;
	ZeroMemory(&tempP, sizeof(tempP));
	monitorRunning = true;
	DWORD      dwBytesReturned = 0;
	while(!processMonitorThread->shouldStop() && isMonitorRunning())
	{
		WaitForSingleObject(hEvent, INFINITE);
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
				std::wstring processPath;
				std::wstring processModuleName;
				std::wstring parentProcessPath;
				std::wstring parentProcessModuleName;
				std::wstring time = Time::timefieldToString(p.time);

				ProcessManager::getInstance()->onProcessEvent(p.bCreate, time, p.ParentId, 
					p.ProcessId, p.processPath);

				// Get process name and path
				processModuleName = ProcessManager::getInstance()->getProcessModuleName(p.ProcessId);
				processPath = ProcessManager::getInstance()->getProcessPath(p.ProcessId);
				
				// Get parent process name and path
				parentProcessModuleName = ProcessManager::getInstance()->getProcessModuleName(p.ParentId);
				parentProcessPath = ProcessManager::getInstance()->getProcessPath(p.ParentId);
				if(!Monitor::isEventAllowed(processModuleName,parentProcessModuleName,processPath))
				{
					
					DebugPrint(L"Capture-ProcessMonitor: %i %i:%ls -> %i:%ls\n", p.bCreate, p.ParentId, parentProcessPath.c_str(), p.ProcessId, processPath.c_str()); 
					signalProcessEvent(p.bCreate, time, p.ParentId, parentProcessPath, p.ProcessId, processPath);
				}
				tempP = p;
			}		
		}
	}
	SetEvent(hMonitorStoppedEvent);
}
