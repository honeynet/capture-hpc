#include "Precompiled.h"

#include "ProcessManager.h"
#include <Psapi.h>
#include <tlhelp32.h>

ProcessManager::ProcessManager(void)
{
	hProcessCreated = CreateEvent(NULL,FALSE,TRUE,NULL);
	cacheHits = 0;
	cacheMisses = 0;
	cacheFailures = 0;
	initialiseProcessMap();
}

ProcessManager::~ProcessManager(void)
{
	processMonitor = NULL;
		
	LOG(INFO, "Process Manager Statistics\n");
	LOG(INFO, "\tCache Hits: %i\n", cacheHits);
	LOG(INFO, "\tCache Misses: %i\n", cacheMisses);
	LOG(INFO, "\tCache Failures: %i\n", cacheFailures);
	instanceCreated = false;
	processMap.clear();
	CloseHandle(hProcessCreated);
}

ProcessManager*
ProcessManager::getInstance()
{
	if(!instanceCreated)
	{
		processManager = new ProcessManager();
		instanceCreated = true;	
	}
	return processManager;
}

void
ProcessManager::onProcessEvent(BOOLEAN created, const std::wstring& time, 
							   DWORD parentProcessId,
							   DWORD processId, const std::wstring& process)
{
	// When a process is terminated we do not delete it from the processMap.
	// This is because there may be events that occur from the process after
	// it has terminated (because of the way the kernel driver monitors work)
	// We only delete when we encounter the same process id being created again
	if(created == TRUE)
	{
		insertProcess(processId, convertFileObjectPathToDOSPath(process));
	}
}

void 
ProcessManager::insertProcess(DWORD processId, const std::wstring& processPath)
{
	LOG(INFO, "ProcessManager::insertProcess start\n");
	stdext::hash_map<DWORD, std::wstring>::iterator it;
	it = processMap.find(processId);
	if(it !=  processMap.end()) {
		processMap.erase(it);
	}
	std::wstring processCompletePath = convertFileObjectPathToDOSPath(processPath);
	LOG(INFO, "Capture-ProcessManager: Insert process %i -> %ls\n", processId, processCompletePath.c_str()); 
	processMap.insert(std::pair<DWORD, std::wstring>(processId, processCompletePath));
	SetEvent(hProcessCreated);
	LOG(INFO, "ProcessManager::insertProcess end\n");
}

std::wstring
ProcessManager::convertDevicePathToDOSDrive(const std::wstring& devicePath)
{
	wchar_t drive = L'A';
	while(drive <= L'Z')
	{
		wchar_t szDriveName[3] = {drive,L':',L'\0'};
		wchar_t szTarget[512] = {0};
		if(QueryDosDevice(szDriveName, szTarget, 511) != 0)
			if(wcscmp(devicePath.c_str(), szTarget) == 0)
				return szDriveName;
		drive++;
	}
	return L"?:";
}

std::wstring
ProcessManager::convertFileObjectPathToDOSPath(const std::wstring& fileObjectPath)
{
	size_t startsWith = fileObjectPath.find(L'\\');
	if(startsWith==0) {
		if(fileObjectPath.length() > 23)
		{
			wchar_t szTemp[2048];
			size_t pathOffset = fileObjectPath.find(L'\\', fileObjectPath.find(L'\\',1)+1);
			std::wstring devicePath = fileObjectPath.substr(0, pathOffset);
			std::wstring dosDrive = convertDevicePathToDOSDrive(devicePath);
			std::wstring dosPath = dosDrive;
			dosPath += fileObjectPath.substr(pathOffset);
			if(GetLongPathName(dosPath.c_str(),szTemp,2048) > 0)
			{
				dosPath = szTemp;
			}
			return dosPath;
		}
	}
	return fileObjectPath;
}

DWORD ProcessManager::getParentProcessId(DWORD processId) {
	HANDLE hProcessSnap;
	HANDLE hProcess;
	PROCESSENTRY32 pe32;
	DWORD dwParentProcessId = -1;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( hProcessSnap == INVALID_HANDLE_VALUE )
	{
		return -1;
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof( PROCESSENTRY32 );

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if( !Process32First( hProcessSnap, &pe32 ) )
	{
		CloseHandle( hProcessSnap );          // clean the snapshot object
		return -1;
	}

	// Now walk the snapshot of processes, and
	// display information about each process in turn
	do
	{
		hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID );
		if( hProcess != NULL ) {
			CloseHandle( hProcess );
		}

		if(pe32.th32ProcessID==processId) {
			dwParentProcessId = pe32.th32ParentProcessID;
		}
	} while( Process32Next( hProcessSnap, &pe32 ) );

	CloseHandle( hProcessSnap );
	return dwParentProcessId;
}

void ProcessManager::setProcessMonitor(ProcessMonitor* pm) {
	processMonitor = pm;
}

std::wstring
ProcessManager::getProcessPath(DWORD processId)
{
	std::wstring processPath = L"UNKNOWN";
	if(processId == 4)
	{
		processPath = L"System";
		return processPath;
	}

	int retryCount = 10;
	stdext::hash_map<DWORD, std::wstring>::iterator it;

	// Get the process name from the map
	// If its not there wait for the process created event to
	// be signalled and try again up to a certain amount of time
	// If this fails the old way of using windows API to determine
	// the name of the process is used.
	do
	{
		it = processMap.find(processId);
		if(it ==  processMap.end())
		{
			// Wait till a process has been entered
			WaitForSingleObject(hProcessCreated, 100);
		}
		else
		{
			cacheHits++;
			return it->second;
		}

	} while(--retryCount != 0);

	// NOTE testing showed that this case should never or hardly ever
	// occur. If you are getting a lot of cache misses maybe there is
	// something wrong during initialiseProcessMap() for example you
	// do not have the debug privilege.
	cacheMisses++;
	wchar_t szTemp[1024];
	ZeroMemory(&szTemp, sizeof(szTemp));

	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION |
                        PROCESS_VM_READ, FALSE, processId);
	if(!hProc)
	{
		cacheFailures++;
		LOG(INFO, "Capture-ProcessManager: Cache failure1 - process id = %i\n", processId); 
		processPath = L"UNKNOWN";
		return convertFileObjectPathToDOSPath(processPath);
	}
	// Cannot use GetModuleFileName because the module list won't be
	// updated in time ... only happens on rare occasions.
	DWORD ret = GetProcessImageFileName(hProc, szTemp,1024);
	CloseHandle(hProc);
	if(ret > 0)
	{
		processPath = szTemp;
		//this occurs when the process monitor is slower than the registry/file monitor
		LOG(INFO, "Capture-ProcessManager: Cache miss %i -> %ls\n", processId, processPath.c_str()); 
		insertProcess(processId, processPath);

		if(processMonitor != NULL) {
			DWORD parentProcessId = getParentProcessId(processId);
			SYSTEMTIME st;
			GetLocalTime(&st);
			std::wstring time = Time::systemtimeToString(st);
			processMonitor->onProcessEvent(true,processId,parentProcessId,time);
		}

	} else {
		cacheFailures++;
		processPath=L"UNKNOWN";
		LOG(INFO, "Capture-ProcessManager: Cache failure2 - process id = %i\n", processId); 
	}
	LOG(INFO, "ProcessManager::getProcessPath end4\n");
	return convertFileObjectPathToDOSPath(processPath);		
}



std::wstring
ProcessManager::getProcessModuleName(DWORD processId)
{
	std::wstring processPath = getProcessPath(processId);
	std::wstring processModuleName = L"UNKNOWN";
	size_t moduleNameStart = processPath.find_last_of(L"\\");
	if(moduleNameStart != std::wstring::npos)
	{
		processModuleName = processPath.substr(moduleNameStart+1);
	}
	return processModuleName;
}

void
ProcessManager::initialiseProcessMap()
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) ) {
		LOG(INFO, "Capture-ProcessManager: Unable to enum processes.\n");
		return;
	}

	cProcesses = cbNeeded / sizeof(DWORD);
	for (UINT i = 0; i < cProcesses; i++ ) 
	{
		if( aProcesses[i] != 0 ) {
			DWORD processId;
			wchar_t processPathTemp[1024];
			std::wstring processPath = L"<unknown>";
			HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                               PROCESS_VM_READ,
                               FALSE, aProcesses[i] );
			processId = aProcesses[i];
			if (NULL != hProcess )
			{
				HMODULE hMod;
				DWORD cbNeeded;

				if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
						&cbNeeded) )
				{
					GetProcessImageFileName(hProcess, processPathTemp, 1024);
					processPath = processPathTemp;
				}
			}
			insertProcess(processId, processPath);
			CloseHandle( hProcess );
		}
	}
}


stdext::hash_map<DWORD, std::wstring> ProcessManager::getProcessMap() { return processMap; }
