/*
 *	PROJECT: Capture
 *	FILE: ProcessHandler.h
 *	AUTHORS: Ramon Steenson (rsteenson@gmail.com) & Christian Seifert (christian.seifert@gmail.com)
 *
 *	Copyright Victoria University of Wellington 2008
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
#include "ProcessHandler.h"


ProcessHandler::ProcessHandler(std::wstring fullPathToExe, std::wstring parameters)
{
	m_fullPathToExe = fullPathToExe;
	
	size_t iPos = 0;
	std::wstring sTempStr = L"";
	m_parameters = parameters;
	/* Add a space to the beginning of the Parameters */
    if (m_parameters.size() != 0)
     {
          if (m_parameters[0] != L' ')
          {
               m_parameters.insert(0,L" ");
          }
    }

	/* The first parameter needs to be the exe itself */
     sTempStr = m_fullPathToExe;
     iPos = sTempStr.find_last_of(L"\\");
     sTempStr.erase(0, iPos +1);
     m_parameters = sTempStr.append(m_parameters);

	 // allocate mem for process info
	 memset(&m_piProcessInfo, 0, sizeof(m_piProcessInfo));

	
}

ProcessHandler::~ProcessHandler(void)
{
	CloseHandle(m_piProcessInfo.hProcess);
    CloseHandle(m_piProcessInfo.hThread);
}

DWORD ProcessHandler::closeProcess(void) {


	DWORD iReturnVal = 0;
	
	//first the process denoted by this process handler
	EnumWindows(ProcessHandler::EnumWindowsCloseProc, (LPARAM) this);
	//lets try to terminate
	if(!TerminateProcess(m_piProcessInfo.hProcess, 0))
	{
		iReturnVal = GetLastError();	
	}

	//then all processes that match the exe
	DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
        iReturnVal = -1;

    // Calculate how many process identifiers were returned.
    cProcesses = cbNeeded / sizeof(DWORD);

    for ( i = 0; i < cProcesses; i++ )
	{
        if( aProcesses[i] != 0 )
		{
			std::wstring processName = m_fullPathToExe;
			size_t iPos = processName.find_last_of(L"\\");
			processName.erase(0, iPos +1);
			//wprintf(processName.c_str());

            if(compareName(aProcesses[i], processName)==0) 
			{
				EnumWindows(ProcessHandler::EnumWindowsCloseAppProc, (LPARAM) aProcesses[i]);
				
				HANDLE hPro = OpenProcess(PROCESS_TERMINATE,TRUE, aProcesses[i]);
				if(!TerminateProcess(hPro, 0))
				{
					iReturnVal = GetLastError();	
				}
			}
		}
	}
	
	return iReturnVal;
}

bool ProcessHandler::isOpen()
{
	bool open = false;

	DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
        return open;

    // Calculate how many process identifiers were returned.

    cProcesses = cbNeeded / sizeof(DWORD);

    // Print the name and process identifier for each process.

    for ( i = 0; i < cProcesses; i++ )
	{
        if( aProcesses[i] != 0 )
		{
			std::wstring processName = m_fullPathToExe;
			size_t iPos = processName.find_last_of(L"\\");
			processName.erase(0, iPos +1);
			//wprintf(processName.c_str());

           if(compareName(aProcesses[i], processName)==0) 
			{
				open = true;
			}

		}
	}
	return open;
}

//lookup process name of process with processID
//compare to processName
int ProcessHandler::compareName(DWORD processID, std::wstring processName)
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
    }

	//_tprintf( TEXT("%s  (PID: %u)\n"), szProcessName, processID );
	int comparison;
	comparison = wcsicmp(szProcessName, processName.c_str());
    CloseHandle( hProcess );

	return comparison;
}

void ProcessHandler::DebugPrintTrace(LPCTSTR pszFormat, ... )
{
	
		wchar_t szOutput[MAX_PATH * 2];
		va_list argList;
		va_start(argList, pszFormat);
		StringCchVPrintf(szOutput, MAX_PATH*2, pszFormat, argList);
		va_end(argList);
		printf("%ls\n", szOutput);
		//OutputDebugString(szOutput);
	
	
}

DWORD ProcessHandler::getProcessId(void) 
{
	return m_piProcessInfo.dwProcessId;
}



BOOL CALLBACK ProcessHandler::EnumWindowsCloseProc(HWND hwnd,LPARAM lParam)
{
	ProcessHandler* processHandler = (ProcessHandler*) lParam;
	DWORD processId = processHandler->getProcessId();
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

BOOL CALLBACK ProcessHandler::EnumWindowsCloseAppProc(HWND hwnd,LPARAM lParam)
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

DWORD ProcessHandler::executeProcess() 
{
	size_t iMyCounter = 0; 
	DWORD iReturnVal = 0; 
    DWORD dwExitCode = 0;
     

     /*
          CreateProcessW can modify Parameters thus we
          allocate needed memory
     */
     wchar_t * pwszParam = new wchar_t[m_parameters.size() + 1];
     if (pwszParam == 0)
     {
          return 1;
     }
     const wchar_t* pchrTemp = m_parameters.c_str();
     wcscpy_s(pwszParam, m_parameters.size() + 1, pchrTemp);

     /* CreateProcess API initialization */
     STARTUPINFOW siStartupInfo;
     memset(&siStartupInfo, 0, sizeof(siStartupInfo));
     siStartupInfo.cb = sizeof(siStartupInfo);

     if (!CreateProcessW(const_cast<LPCWSTR>(m_fullPathToExe.c_str()),
                                  pwszParam, 0, 0, true,
                                  CREATE_DEFAULT_ERROR_MODE, 0, 0,
                                  &siStartupInfo, &m_piProcessInfo))
     {
		 /* CreateProcess failed */
          iReturnVal = GetLastError();
	 } else {
		 WaitForInputIdle(m_piProcessInfo.hProcess, 2000);
		 Sleep(1000);
	 }

     /* Free memory */
     delete[]pwszParam;
     pwszParam = 0;



     return iReturnVal;
}