#include "Precompiled.h"

#include "ProcessMonitor.h"
#include "EventController.h"
#include "ProcessManager.h"

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
			LOG(INFO, "ProcessMonitor: ERROR - CreateFile Failed: %08x", GetLastError());
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
}

void
ProcessMonitor::initialiseKernelDriverProcessMap()
{
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
		CloseHandle(hEvent);

		processMonitorThread->exit();
		delete processMonitorThread;
	}
}

void ProcessMonitor::onProcessEvent(BOOLEAN create, DWORD processId, DWORD parentProcessId, std::wstring time) {
	std::wstring processPath;
	std::wstring processModuleName;
	std::wstring parentProcessPath;
	std::wstring parentProcessModuleName;
	
	processPath = ProcessManager::getInstance()->getProcessPath(processId);
	
	ProcessManager::getInstance()->onProcessEvent(create, time, parentProcessId, 
		processId, processPath);

	// Get process name and path
	processModuleName = ProcessManager::getInstance()->getProcessModuleName(processId);
	
	// Get parent process name and path
	parentProcessModuleName = ProcessManager::getInstance()->getProcessModuleName(parentProcessId);
	parentProcessPath = ProcessManager::getInstance()->getProcessPath(parentProcessId);
	LOG(INFO, "received process event %i %i:%ls -> %i:%ls", create, parentProcessId, parentProcessPath.c_str(), processId, processPath.c_str()); 
	if(!Monitor::isEventAllowed(processModuleName,parentProcessModuleName,processPath))
	{
		signalProcessEvent(create, time, parentProcessId, parentProcessPath, processId, processPath);
	}
}


void
ProcessMonitor::run()
{
	ProcessInfo tempP;
	ZeroMemory(&tempP, sizeof(tempP));
	monitorRunning = true;
	DWORD      dwBytesReturned = 0;
	while(running && isMonitorRunning())
	{
		DWORD status = WaitForSingleObject(hEvent, 1000);
		if(status == WAIT_TIMEOUT)
			continue;

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
				std::wstring time = Time::timefieldToString(p.time);
				DWORD processId = p.ProcessId;
				DWORD parentProcessId = p.ParentId;
				BOOLEAN create = p.bCreate;
				
				onProcessEvent(create,processId,parentProcessId,time);

				tempP = p;
			}
		}
	}
}
