/*
 *	PROJECT: Capture
 *	FILE: Client_InternetExplorer.cpp
 *	AUTHORS: Ramon Steenson (rsteenson@gmail.com) & Christian Seifert (christian.seifert@gmail.com)
 *
 *	Developed by Victoria University of Wellington and the New Zealand Honeynet Alliance
 *
 *	This file is part of Capture.
 *
 *	Capture is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Capture is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Capture; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "Application_InternetExplorer.h"
#include "InternetExplorerInstance.h"

static HANDLE worker_threads[MAX_WORKER_THREADS]; 
static bool worker_thread_busy[MAX_WORKER_THREADS];
static LPVOID* worker_thread_data[MAX_WORKER_THREADS];
static HANDLE worker_has_data[MAX_WORKER_THREADS];
static HANDLE worker_finished[MAX_WORKER_THREADS]; 

struct VISIT_INFO
{
	InternetExplorerInstance* internet_explorer_instance;
	Url* url;
};

DWORD WINAPI 
Application_InternetExplorer::InternetExplorerWorker(LPVOID data)
{
	DebugPrintTrace(L"Application_InternetExplorer::InternetExplorerWorker start\n");
	int worker_id = (int)data;
	DebugPrint(L"IE Worker Start\n.");
	while(true)
	{
		DebugPrint(L"IE Worker Visit Start.\n");
		
		WaitForSingleObject(worker_has_data[worker_id], INFINITE);
		

		VISIT_INFO* visit_information = (VISIT_INFO*)worker_thread_data[worker_id];

		_ASSERT(visit_information);

		if(visit_information)
		{
			// Get the visit information
			Url* url = visit_information->url;
			InternetExplorerInstance* internet_explorer_instance = visit_information->internet_explorer_instance;
			
			// Visit the actual url
			internet_explorer_instance->visitUrl(url);
			worker_thread_busy[worker_id] = false;
			SetEvent(worker_finished[worker_id]);
		}
		DebugPrint(L"IE Worker Visit End.\n");
	}
	DebugPrint(L"IE Worker End\n.");

	DebugPrintTrace(L"Application_InternetExplorer::InternetExplorerWorker end\n");
}

Application_InternetExplorer::Application_InternetExplorer()
{
	DebugPrintTrace(L"Application_InternetExplorer::Application_InternetExplorer() start\n");
	
	// Start the COM interface
	CoInitializeEx(NULL,COINIT_MULTITHREADED);

	// Reset worker threads status
	for(unsigned int i = 0; i < MAX_WORKER_THREADS; i++)
	{
		worker_thread_busy[i] = false;
		worker_thread_data[i] = NULL;
		worker_has_data[i] = CreateEvent(NULL, false, NULL, NULL);
		worker_finished[i] = CreateEvent(NULL, false, NULL, NULL); 
		worker_threads[i] = CreateThread(NULL, 0, &Application_InternetExplorer::InternetExplorerWorker, (LPVOID)i, 0, NULL);
	}
	DebugPrintTrace(L"Application_InternetExplorer::Application_InternetExplorer() end\n");
}

Application_InternetExplorer::~Application_InternetExplorer(void)
{
	DebugPrintTrace(L"Application_InternetExplorer::~Application_InternetExplorer(void) start\n");
		
	for(unsigned int i = 0; i < MAX_WORKER_THREADS; i++)
	{
		CloseHandle(worker_has_data[i]);
		CloseHandle(worker_finished[i]);
		CloseHandle(worker_threads[i]);
	}

	CoUninitialize();
	DebugPrintTrace(L"Application_InternetExplorer::~Application_InternetExplorer(void) end\n");
	

}



void
Application_InternetExplorer::visitGroup(VisitEvent* visitEvent)
{	
	DebugPrintTrace(L"Application_InternetExplorer::visitGroup(VisitEvent* visitEvent) start\n");

	DWORD *error = NULL;
	
	unsigned int n_visited_urls = 0;
	unsigned int to_visit = 0;
	unsigned int n_visiting = 0;
	int n_urls = visitEvent->getUrls().size();



	IClassFactory* internet_explorer_factory;

	// Get a link to the IE factory for creating IE instances
	HRESULT hr = CoGetClassObject(CLSID_InternetExplorer,
		CLSCTX_LOCAL_SERVER,
		NULL,
		IID_IClassFactory,
		(void**)&internet_explorer_factory); 

	// Allocate on the heap so threads can use the information
	InternetExplorerInstance** iexplore_instances = (InternetExplorerInstance**)malloc(sizeof(InternetExplorerInstance*)*n_urls);
	for(unsigned int i = 0; i < n_urls; i++)
	{
		iexplore_instances[i] = new InternetExplorerInstance(internet_explorer_factory);
	}

	VISIT_INFO* visit_information = new VISIT_INFO[n_urls];

	// Loop until all urls have been visited
	while(n_visited_urls < n_urls)
	{
		for(unsigned int i = 0; i < MAX_WORKER_THREADS; i++)
		{
			if(!worker_thread_busy[i] && n_visiting < n_urls)
			{
				// Give the threads something to do					
				visit_information[to_visit].internet_explorer_instance = iexplore_instances[to_visit];
				visit_information[to_visit].url = visitEvent->getUrls().at(to_visit);
				worker_thread_data[i] = (LPVOID*)&visit_information[to_visit++];
				worker_thread_busy[i] = true;
				n_visiting++;
				SetEvent(worker_has_data[i]);	
			}
		}

		// Wait for one of the workers threads to finish
		DWORD dwWait = WaitForMultipleObjects(MAX_WORKER_THREADS, worker_finished, false, 60*1000);
		DebugPrint(L"IE Visit Group Worker Threads Finished.\n");

		// If one has finished then a url has been visited
		int index = dwWait - WAIT_OBJECT_0;
		if(index < MAX_WORKER_THREADS && !worker_thread_busy[index])
		{
			n_visited_urls++;
		}
	}
	DebugPrint(L"IE: Finished visiting %i URLs\n", n_visited_urls);		

	// Give the visit event a success or error code based on the visitaion of each url
	for(unsigned int i = 0; i < n_urls; i++)
	{
		Url* url = visitEvent->getUrls().at(i);
		visitEvent->setErrorCode(url->getMajorErrorCode());
	}
	DebugPrint(L"IE Visit Group set errors on urls.\n");

	DWORD errorCode = closeAllInternetExplorers(internet_explorer_factory);
	if(errorCode!=0) {
		visitEvent->setErrorCode( CAPTURE_VISITATION_WARNING );
	}
	DebugPrint(L"IE Visit Group closed all instances.\n");


	//Delete all IE instance objects
	delete [] visit_information;
	for(unsigned int i = 0; i < n_urls; i++)
	{
		delete iexplore_instances[i];
	}
	free(iexplore_instances);
	DebugPrint(L"IE Visit Group delete IE instances\n");

	// Free the COM interface stuff
	ULONG num_references = internet_explorer_factory->Release();

	DebugPrintTrace(L"Application_InternetExplorer::visitGroup(VisitEvent* visitEvent) end\n");
}

DWORD Application_InternetExplorer::closeAllInternetExplorers(IClassFactory* internet_explorer_factory) {

	DebugPrintTrace(L"Application_InternetExplorer::closeAllInternetExplorers(IClassFactory* internet_explorer_factory) start\n");
	DWORD iReturnVal;
	iReturnVal = 0;
	// Create another fake IE instance so that we can close the process
	
	IWebBrowser2* pInternetExplorer;
	HRESULT hr = internet_explorer_factory->CreateInstance(NULL, IID_IWebBrowser2, 
							(void**)&pInternetExplorer);
	
	if( hr == S_OK )
	{
		DebugPrint(L"IE CloseAll created fake IE instance.\n");
		HWND hwndIE;
		DWORD dProcessId;
		pInternetExplorer->get_HWND((SHANDLE_PTR*)&hwndIE);
		
		GetWindowThreadProcessId(hwndIE, &dProcessId);
		// Close the IE process - try 1
		EnumWindows(Application_InternetExplorer::EnumWindowsProc, (LPARAM)dProcessId);
	
		// Close the IE process - try 2
		HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, dProcessId);
		DWORD tempProcessId = GetProcessId(hProc);
		if(tempProcessId == dProcessId)
		{
			if(!TerminateProcess(hProc, 0))
			{
				iReturnVal = GetLastError();
				DebugPrint(L"IE CloseAll unable to terminate 1.\n");
			}
		}

		if( hProc != NULL) {
			CloseHandle( hProc );
		}
		if (hwndIE !=NULL) {
			CloseHandle( hwndIE );
		}
	}
	if(pInternetExplorer!=NULL) {
		pInternetExplorer->Release();
	}
	

	//then all processes that match the exe
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) ) {
		DebugPrint(L"IE CloseAll couldn't enum processes.\n");
		iReturnVal = -1;
	}

	// Calculate how many process identifiers were returned.
	cProcesses = cbNeeded / sizeof(DWORD);

	for ( i = 0; i < cProcesses; i++ )
	{
		if( aProcesses[i] != 0 )
		{
			std::wstring processName = L"c:\\Program Files\\Internet Explorer\\iexplore.exe";
			size_t iPos = processName.find_last_of(L"\\");
			processName.erase(0, iPos +1);
			
			if(compareName(aProcesses[i], processName)==0) 
			{
				DebugPrint(L"IE CloseAll IE process left over. Closing....\n");
				EnumWindows(Application_InternetExplorer::EnumWindowsCloseAppProc, (LPARAM) aProcesses[i]);
				
				HANDLE hPro = OpenProcess(PROCESS_TERMINATE,TRUE, aProcesses[i]);
				if(!TerminateProcess(hPro, 0))
				{
					iReturnVal = GetLastError();
					DebugPrint(L"IE CloseAll unable to terminate 2.\n");
				}

				if( hPro != NULL ) {
					CloseHandle (hPro);
				}
			}
		}
	}

	DebugPrintTrace(L"Application_InternetExplorer::closeAllInternetExplorers(IClassFactory* internet_explorer_factory) end\n");
	return iReturnVal;
}


//lookup process name of process with processID
//compare to processName
int Application_InternetExplorer::compareName(DWORD processID, std::wstring processName)
{
	TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
    // Get a handle to the process.

    HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_READ,
                                   FALSE, processID );

    // Get the process name.

    if (NULL != hProcess )
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
             &cbNeeded) )
        {
            GetModuleBaseName( hProcess, hMod, szProcessName, 
                               sizeof(szProcessName)/sizeof(TCHAR) );
        }
		
		CloseHandle( hProcess );
    }

	//_tprintf( TEXT("%s  (PID: %u)\n"), szProcessName, processID );
	int comparison;
	comparison = wcsicmp(szProcessName, processName.c_str());
    

	return comparison;
}

wchar_t**
Application_InternetExplorer::getSupportedApplicationNames()
{
	DebugPrintTrace(L"Application_InternetExplorer::getSupportedApplicationNames() start\n");
	DebugPrintTrace(L"Application_InternetExplorer::getSupportedApplicationNames() end\n");
	return supportedApplications;
}


BOOL CALLBACK Application_InternetExplorer::EnumWindowsProc(HWND hwnd,LPARAM lParam)
{
	DWORD processId = (DWORD) lParam;
	if (GetWindowLong(hwnd,GWL_STYLE) & WS_VISIBLE) {
		DWORD windowsProcessId;
		GetWindowThreadProcessId(hwnd, &windowsProcessId);
		if (windowsProcessId == processId) 
		{
			SendMessage(hwnd, WM_CLOSE, 1, 0);		
		}
	}
	return TRUE;
}

BOOL CALLBACK Application_InternetExplorer::EnumWindowsCloseAppProc(HWND hwnd,LPARAM lParam)
{
	DWORD processId = (DWORD) lParam;
	if (GetWindowLong(hwnd,GWL_STYLE) & WS_VISIBLE) {
		DWORD windowsProcessId;
		GetWindowThreadProcessId(hwnd, &windowsProcessId);
		if (windowsProcessId == processId) 
		{
			SendMessage(hwnd, WM_CLOSE, 1, 0);		
		}
	}
	return TRUE;
}


