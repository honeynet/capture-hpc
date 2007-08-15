/*
 *	PROJECT: Capture
 *	FILE: ProcessMonitor.cpp
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
#include "StdAfx.h"
#include "ProcessMonitor.h"

ProcessMonitor::ProcessMonitor(Visitor* v)
{
	wchar_t kernelDriverPath[1024];
	wchar_t exListDriverPath[1024];

	installed = false;
	hEvent = INVALID_HANDLE_VALUE;
	v->AddVisitorListener(this);
	
	// Load exclusion list
	GetCurrentDirectory(1024, exListDriverPath);
	wcscat_s(exListDriverPath, 1024, L"\\ProcessMonitor.exl");
	Monitor::LoadExclusionList(exListDriverPath);

	// Load process monitor kernel driver
	GetCurrentDirectory(1024, kernelDriverPath);
	wcscat_s(kernelDriverPath, 1024, L"\\CaptureProcessMonitor.sys");
	if(Monitor::InstallKernelDriver(kernelDriverPath, L"CaptureProcessMonitor", L"Capture Process Monitor"))
	{	
		hDriver = CreateFile(
					L"\\\\.\\Global\\CaptureProcessMonitor",
					GENERIC_READ | GENERIC_WRITE, 
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					0,                     // Default security
					OPEN_EXISTING,
					FILE_FLAG_OVERLAPPED,  // Perform asynchronous I/O
					0);                    // No template
		if(INVALID_HANDLE_VALUE == hDriver) {
			printf("ProcessMonitor: ERROR - CreateFile Failed: %x\n", GetLastError());
		} else {
			installed = true;
		}
	}

	if(installed)
	{
		DWORD aProcesses[1024], cbNeeded, cProcesses;
		unsigned int i;

		if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
			return;

		// Calculate how many process identifiers were returned.

		cProcesses = cbNeeded / sizeof(DWORD);

		// Print the name and process identifier for each process.
		//PROCESS_TUPLE pTuple[cProcesses];
		for ( i = 0; i < cProcesses; i++ ) 
		{
			if( aProcesses[i] != 0 ) {
				//PrintProcessNameAndID( aProcesses[i] );
				TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

				// Get a handle to the process.

				HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_READ,
                                   FALSE, aProcesses[i] );

				// Get the process name.
				PROCESS_TUPLE pTuple;
				//char *szMsg = (char*)malloc(msg.length());
				//PPROCESS_TUPLE pTuple = (PPROCESS_TUPLE)malloc(sizeof(PROCESS_TUPLE));
				ZeroMemory(&pTuple, sizeof(PROCESS_TUPLE));
				//pTuple.name = L"<unknown>";
				pTuple.processID = aProcesses[i];
				if (NULL != hProcess )
				{
					HMODULE hMod;
					DWORD cbNeeded;

					if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
							&cbNeeded) )
					{
						GetProcessImageFileName(hProcess, pTuple.name, sizeof(pTuple.name)/sizeof(TCHAR));
						DWORD dwReturn;
		
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

			
				CloseHandle( hProcess );
			}
		}
	}
}

ProcessMonitor::ProcessMonitor()
{
	wchar_t kernelDriverPath[1024];

	GetCurrentDirectory(1024, kernelDriverPath);
	wcscat_s(kernelDriverPath, 1024, L"\\CaptureProcessMonitor.sys");
	if(Monitor::InstallKernelDriver(kernelDriverPath, L"CaptureProcessMonitor", L"Capture Process Monitor"))
	{	
		hDriver = CreateFile(
					L"\\\\.\\Global\\CaptureProcessMonitor",
					GENERIC_READ | GENERIC_WRITE, 
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					0,                     // Default security
					OPEN_EXISTING,
					FILE_FLAG_OVERLAPPED,  // Perform asynchronous I/O
					0);                    // No template
		if(INVALID_HANDLE_VALUE == hDriver) {
			printf("ProcessMonitor: ERROR - CreateFile Failed: %i\n", GetLastError());
		} else {
			installed = true;
		}
	}
}

ProcessMonitor::~ProcessMonitor(void)
{
	if(hEvent != INVALID_HANDLE_VALUE)
		CloseHandle(hEvent);
	if(hDriver != INVALID_HANDLE_VALUE)
		CloseHandle(hDriver);
	Monitor::UnInstallKernelDriver();
}

void
ProcessMonitor::Start()
{
	if(!installed)
	{
		printf("ProcessMonitor: ERROR - Kernel driver was not loaded properly\n");
		return;
	}
    				
	hEvent = OpenEvent(SYNCHRONIZE, FALSE, L"Global\\CaptureProcDrvProcessEvent");

	if(!hEvent)
	{
		printf("ProcessMonitor: ERROR - OpenEvent Failed: %x\n", GetLastError());
		return;
	}

	
	procmonThread = new Thread(this);
	procmonThread->Start();
}

void
ProcessMonitor::Stop()
{
	procmonThread->Stop();
//	regmon->Stop();
//	UnInstallKernelDriver();	
}

void 
ProcessMonitor::Pause()
{
	BOOL       bReturnCode = FALSE;
	DWORD      dwBytesReturned = 0;

	bReturnCode = DeviceIoControl(
		hDriver,
		IOCTL_CAPTURE_STOP,
		0, 
		0,
		0, 
		0,
		&dwBytesReturned,
		NULL
		);
}

void 
ProcessMonitor::UnPause()
{
	BOOL       bReturnCode = FALSE;
	DWORD      dwBytesReturned = 0;

	bReturnCode = DeviceIoControl(
		hDriver,
		IOCTL_CAPTURE_START,
		0, 
		0,
		0, 
		0,
		&dwBytesReturned,
		NULL
		);
}

void
ProcessMonitor::Run()
{
	ProcessInfo tempP;
	ZeroMemory(&tempP, sizeof(tempP));

	while(true)
	{
		WaitForSingleObject(hEvent, INFINITE);

		BOOL       bReturnCode = FALSE;
		DWORD      dwBytesReturned = 0;
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
				wstring processPath = LogicalToDOSPath(p.procPath);
				wstring processModuleName;
				wstring parentProcessPath = LogicalToDOSPath(p.parentProcPath);
				wstring parentProcessModuleName;

				// Get process name and path
				//if(Monitor::GetProcessCompletePathName((DWORD)p.ProcessId, &processPath, p.bCreate))
				//{
					processModuleName = processPath.substr(processPath.find_last_of(L"\\")+1);
				//}

				// Get parent process name and path
				//if(Monitor::GetProcessCompletePathName((DWORD)p.ParentId, &parentProcessPath, true))
				//{
					parentProcessModuleName = parentProcessPath.substr(parentProcessPath.find_last_of(L"\\")+1);
				//}
				
				//printf("PP: %ls -> %ls\n", LogicalToDOSPath(p.parentProcPath).c_str(), p.procPath);
				if(!CheckIfExcluded(processPath)) 
				{
					if(!Monitor::EventIsAllowed(processModuleName,parentProcessModuleName,processPath))
					{
						wstring time = Monitor::TimeFieldToWString(p.time);

						NotifyListeners(p.bCreate, time, parentProcessPath, processPath);
					} else {
						//printf("excluded: %ls\n", processPath.c_str());
					}
				}		
				tempP = p;
			}
			
		}
	}
}



void
ProcessMonitor::NotifyListeners(BOOLEAN bCreate, wstring time, wstring processParentPath, wstring processPath)
{
	std::list<ProcessListener*>::iterator lit;

	// Inform all registered listeners of event
	for (lit=processListeners.begin(); lit!= processListeners.end(); lit++)
	{
		(*lit)->OnProcessEvent(bCreate, time, processParentPath, processPath);
	}
}

void 
ProcessMonitor::AddProcessListener(ProcessListener* pl)
{
	processListeners.push_back(pl);
}

void
ProcessMonitor::RemoveProcessListener(ProcessListener* pl)
{
	processListeners.remove(pl);
}

void
ProcessMonitor::OnVisitEvent(int eventType, DWORD minorEventCode, wstring url, wstring visitor)
{
	if(eventType == VISIT_START)
	{
		currentVisitingClient = visitor;
	} else {
		currentVisitingClient = L"";
	}
}