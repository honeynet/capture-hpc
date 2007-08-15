/*
 *	PROJECT: Capture
 *	FILE: Monitor.cpp
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
#include "Monitor.h"

wstring 
Monitor::TimeFieldToWString(TIME_FIELDS time)
{
	wchar_t szTime[16];
	ZeroMemory(&szTime, sizeof(szTime));
	wstring wtime;
	_itow_s(time.Day,szTime,16,10);
	wtime += szTime;
	wtime += L"\\";
	_itow_s(time.Month,szTime,16,10);
	wtime += szTime;
	wtime += L"\\";
	_itow_s(time.Year,szTime,16,10);
	wtime += szTime;
	wtime += L" ";
	_itow_s(time.Hour,szTime,16,10);
	wtime += szTime;
	wtime += L":";
	_itow_s(time.Minute,szTime,16,10);
	wtime += szTime;
	wtime += L":";
	_itow_s(time.Second,szTime,16,10);
	wtime += szTime;
	wtime += L".";
	_itow_s(time.Milliseconds,szTime,16,10);
	wtime += szTime;
	return wtime;
}

bool
Monitor::EventIsAllowed(std::wstring eventType, std::wstring subject, std::wstring object)
{
	stdext::hash_map<wstring, std::list<Permission*>*>::iterator it;
	std::transform(eventType.begin(),eventType.end(),eventType.begin(),std::towlower);
	it = permissionMap.find(eventType);
	PERMISSION_CLASSIFICATION excluded = NO_MATCH;
	if(it != permissionMap.end())
	{
		std::list<Permission*>* lp = it->second;
		std::list<Permission*>::iterator lit;

		for(lit = lp->begin(); lit != lp->end(); lit++)
		{
			PERMISSION_CLASSIFICATION newExcluded = (*lit)->Check(subject,object);
			if( newExcluded == ALLOWED)
			{
				if(excluded != DISALLOWED)
					excluded = ALLOWED;
			} else if(newExcluded == DISALLOWED) {
				excluded = DISALLOWED;
			}
		}
	}
	if(excluded == ALLOWED)
		return true;
	else
		return false;
}

wchar_t
Monitor::GetDriveLetter(LPCTSTR lpDevicePath)
{
	wchar_t d = _T('A');
	while(d <= _T('Z'))
	{
		wchar_t szDeviceName[3] = {d,_T(':'),_T('\0')};
		wchar_t szTarget[512] = {0};
		if(QueryDosDevice(szDeviceName, szTarget, 511) != 0)
			if(_tcscmp(lpDevicePath, szTarget) == 0)
				return d;
		d++;
	}
	return NULL;
}

bool
Monitor::GetProcessCompletePathName(DWORD pid, std::wstring *path, bool created)
{
	wchar_t szTemp[512];
	bool nameFound = false;
	stdext::hash_map<DWORD, wstring>::iterator it;

	if(pid == 4)
	{
		// System process
		*path = L"System";
		return true;
	}

	it = processNameMap.find(pid);
	if(it ==  processNameMap.end())
	{
		ZeroMemory(&szTemp, sizeof(szTemp));

		HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION |
                            PROCESS_VM_READ, FALSE, pid);

		// Cannot use GetModuleFileName because the module list won't be
		// updated in time ... only happens on rare occasions.
		if(!hProc)
			return false;
		DWORD ret = GetProcessImageFileName(hProc, szTemp,512);
		CloseHandle(hProc);
		if(ret > 0)
		{
			// GetProcessImageFileName returns the logical path ...
			// convert it to the DOS path c:\ ... etc
			*path = LogicalToDOSPath(szTemp);
			processNameMap.insert(Process_Pair(pid, *path));
			nameFound = true;
		}
	} else {
		*path = it->second;
		if(!created)
		{
			processNameMap.erase(it);
		}
		nameFound = true;
	}
	return nameFound;
}

bool
Monitor::InstallKernelDriver(std::wstring driverFullPathName, std::wstring driverName, std::wstring driverDescription)
{
	SC_HANDLE hSCManager;

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

    if(hSCManager)
    {
		printf("%ls: Kernel driver path: %ls\n", driverName.c_str(), driverFullPathName.c_str());
        hService = CreateService(hSCManager, driverName.c_str(), 
								  driverDescription.c_str(), 
                                  SERVICE_START | DELETE | SERVICE_STOP, 
                                  SERVICE_KERNEL_DRIVER,
                                  SERVICE_DEMAND_START, 
                                  SERVICE_ERROR_IGNORE, 
								  driverFullPathName.c_str(), 
                                  NULL, NULL, NULL, NULL, NULL);

        if(!hService)
        {
			hService = OpenService(hSCManager, driverName.c_str(), 
                       SERVICE_START | DELETE | SERVICE_STOP);
        }

        if(hService)
        {
            StartService(hService, 0, NULL);           
		} else {
			CloseServiceHandle(hSCManager);
			return false;
		}

        CloseServiceHandle(hSCManager);
		return true;
    }
	return false;
}

wstring
Monitor::LogicalToDOSPath(wchar_t *logicalPath)
{
	wstring procLogicalPath = logicalPath;
	wstring device = procLogicalPath.substr(0,procLogicalPath.find(L'\\', procLogicalPath.find(L'\\',1)+1));
	wstring szDrive = L"0:";
	wchar_t drive = GetDriveLetter(device.c_str());				
	szDrive[0] = drive;
	wstring procDosPath = procLogicalPath.substr(23);
	szDrive.append(procDosPath);

	return szDrive;
}

bool
Monitor::LoadExclusionList(std::wstring file)
{
	string line;
	int lineNumber = 0;
		
	ifstream exclusionList (file.c_str());
	if (exclusionList.is_open())
	{
		while (! exclusionList.eof() )
		{
			getline (exclusionList,line);
			lineNumber++;
			if(line.length() > 0) {
				
				try {
					if(line.at(0) != '#')
					{
						vector<std::wstring> splitLine;

						typedef split_iterator<string::iterator> sf_it;
						for(sf_it it=make_split_iterator(line, token_finder(is_any_of("\t")));
							it!=sf_it(); ++it)
						{
							splitLine.push_back(copy_range<std::wstring>(*it));				
						}

						if(splitLine.size() == 4)
						{
							Permission* p = new Permission();
							if(splitLine[0] == L"+")
							{
								p->allow = true;
							} else {
								p->allow = false;
							}
							boost::wregex s(splitLine[2].c_str(), boost::regex::icase);
							boost::wregex e(splitLine[3].c_str(), boost::regex::icase);
							p->permissions.push_back(e);
							p->subjects.push_back(s);
							wstring tempEventType = splitLine[1];
							std::transform(tempEventType.begin(),tempEventType.end(),tempEventType.begin(),std::towlower);
							stdext::hash_map<wstring, std::list<Permission*>*>::iterator it;
							it = permissionMap.find(tempEventType);
		
							if(it == permissionMap.end())
							{
								std::list<Permission*>*l = new list<Permission*>();
								l->push_back(p);
								permissionMap.insert(Permission_Pair(tempEventType, l));
							} else {
								std::list<Permission*>* lp = it->second;
								lp->push_back(p);
							}
						}
					}
				} catch(boost::regex_error r)
				{
					printf("%ls -- error on line %i\n", file.c_str(), lineNumber);
				}
			}
		}
	} else {
		printf("Could not open file: %ls\n", file.c_str());
		return false;
	}
	return true;
}

void
Monitor::UnInstallKernelDriver()
{
	SERVICE_STATUS ss;
	ControlService(hService, SERVICE_CONTROL_STOP, &ss);
    DeleteService(hService);
    CloseServiceHandle(hService);	
}