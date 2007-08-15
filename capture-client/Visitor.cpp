/*
 *	PROJECT: Capture
 *	FILE: Visitor.cpp
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
#include "Visitor.h"

Visitor::Visitor(Common* g)
{
	global = g;
	hQueueNotEmpty = CreateEvent(NULL, FALSE, FALSE, NULL);
	wstring sEvent = L"visit";
	
	if(!global->serverEventDelegator->Register(this, sEvent))
	{
		printf("Visitor not listening for visit events from server\n");
	}

	wchar_t exListDriverPath[1024];
	GetCurrentDirectory(1024, exListDriverPath);
	wcscat_s(exListDriverPath, 1024, L"\\Clients.conf");
	wstring configListW = exListDriverPath;

	LoadClientPlugins(L"");
	LoadClientsList(configListW);
	Thread* visit = new Thread(this);	

	visit->Start();
}

Visitor::Visitor()
{
}

Visitor::~Visitor(void)
{
	CloseHandle(hQueueNotEmpty);
	std::list<HMODULE>::iterator lit;
	for(lit = clientPlugins.begin(); lit != clientPlugins.end(); lit++)
	{
		FreeLibrary((*lit));
	}
}

bool
Visitor::LoadClientsList(wstring path)
{
		string line;
		int lineNumber = 0;
		
		ifstream configF (path.c_str());
		if (configF.is_open())
		{
			while (! configF.eof() )
			{
				getline (configF,line);
				lineNumber++;
				if((line.size() > 0) && (line.at(0) != '#'))
				{
					vector<std::wstring> splitLine;

					
					for(sf_it it=make_split_iterator(line, token_finder(is_any_of("\t")));
						it!=sf_it(); ++it)
					{
						splitLine.push_back(copy_range<std::wstring>(*it));				
					}

					if(splitLine.size() >= 2)
					{
						bool downloadURL = false;
						if(splitLine.size() == 3)
						{
							if(splitLine[2] == L"yes")
								downloadURL = true;
						}
						if(visitorClientsMap.find(splitLine[0]) == visitorClientsMap.end())
						{
							Client* c = new Client(splitLine[0],splitLine[1], downloadURL, NULL);

							visitorClientsMap.insert(Client_Pair(splitLine[0], c));
						}
					}
				}
			}
		} else {
			printf("Visitor -- Could not open the client config file: %ls\n", path.c_str());
			return false;
		}
		configF.close();
		return true;
}

void 
Visitor::LoadClientPlugins(wstring directory)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	wchar_t pluginDirectoryPath[1024];
	GetCurrentDirectory(1024, pluginDirectoryPath);
	wcscat_s(pluginDirectoryPath, 1024, L"\\plugins\\Client_*");
	hFind = FindFirstFile(pluginDirectoryPath, &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE) 
	{
		printf ("WARNING - Could not load any plugins: %u\n", GetLastError());
		return;
	} 
	else 
	{
		typedef HRESULT (*CLIENTINTERFACE)(IClient  ** pInterface,stdext::hash_map<wstring, Client*>* visitorClientsMap);
		do
		{
			CLIENTINTERFACE pfnInterface = 0;
			IClient * pInterface = 0;
			wstring pluginDir = L"plugins\\";
			pluginDir += FindFileData.cFileName;
			HMODULE hMod = LoadLibrary(pluginDir.c_str());
			pfnInterface= (CLIENTINTERFACE)GetProcAddress(hMod,"New");
			HRESULT hr = pfnInterface(&pInterface, &visitorClientsMap);
			if(FAILED(hr)) 
			{
				printf("WARNING - Could not load plugin %ls\n", FindFileData.cFileName);
				FreeLibrary(hMod);
			} else {
				clientPlugins.push_back(hMod);
				printf("Loaded plugin: %ls\n", FindFileData.cFileName);
			}
		} while(FindNextFile(hFind, &FindFileData) != 0);
		FindClose(hFind);
	}
}



void
Visitor::Run()
{	
	while(true)
	{
		WaitForSingleObject(hQueueNotEmpty, INFINITE);
		visiting = true;
		Visit_Pair* visit = toVisit.front();
		toVisit.pop();

		Client_Pair client = visit->first;
		Url_Pair* url = visit->second;
		Client* c = client.second;
		bool fileDownloaded = false;
		if(c->downloadURL) {
			FileDownloader* downloader = new FileDownloader();
			wstring downloadedTo;
			if(downloader->Download(url->first, &downloadedTo))
			{		
				url->first = downloadedTo;
				fileDownloaded = true;
			} else {
				//NotifyListeners(VISIT_NETWORK_ERROR, url->first, c->path);
			}
		}

		// Pre event to unpause all of the monitors
		NotifyListeners(VISIT_PRESTART, 0, url->first, c->path);

		NotifyListeners(VISIT_START, 0, url->first, c->path);

		if(c->downloadURL && !fileDownloaded)
		{
			NotifyListeners(VISIT_NETWORK_ERROR, 1, url->first, c->path);
			goto finish;
		}
		
		if(c->application != NULL)
		{
			printf("Visitor: Using plugin\n");
			DWORD minorErrorCode = 0;
			c->application->Open(client.first);
			DWORD majorErrorCode = c->application->Visit(url->first, url->second, &minorErrorCode);
			if(majorErrorCode != 0)
			{
				NotifyListeners(majorErrorCode, minorErrorCode, url->first, c->path);
			}
			c->application->Close();
			if(majorErrorCode == 0)
				NotifyListeners(VISIT_FINISH, 0, url->first, c->path);
		} else {
			printf("Visitor: Using CreateProcess\n");
			STARTUPINFO siStartupInfo;
			PROCESS_INFORMATION piProcessInfo;
			memset(&siStartupInfo, 0, sizeof(siStartupInfo));
			memset(&piProcessInfo, 0, sizeof(piProcessInfo));
			siStartupInfo.cb = sizeof(siStartupInfo);
			// TODO fix this ... shouldn't be a constant value
			//size_t length = 1024;
			wchar_t szTempPath[1024];
			ZeroMemory(&szTempPath, sizeof(szTempPath));
			_tcscat_s(szTempPath, 1024, L"\"");
			_tcscat_s(szTempPath, 1024, c->path.c_str());
			_tcscat_s(szTempPath, 1024, L"\" ");
			_tcscat_s(szTempPath, 1024, url->first.c_str());
			LPTSTR szCmdline=_tcsdup(szTempPath);
			bool created = CreateProcess(NULL,szCmdline, 0, 0, FALSE,
				CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo,
				&piProcessInfo);

			if(!created)
				NotifyListeners(VISIT_PROCESS_ERROR, 0, url->first, c->path);

			Sleep(url->second*1000);
			
			//if(!TerminateProcess(piProcessInfo.hProcess, 0))
			//HWND blah = FindWindowW(L"MozillaUIWindowClass", NULL);
			//PostMessage(blah, WM_CLOSE, 0, 0);
			//if(EnumWindows(EnumWindowsProc, piProcessInfo.dwProcessId))
			if(!TerminateProcess(piProcessInfo.hProcess, 0))
			{
				NotifyListeners(VISIT_PROCESS_ERROR, 0, url->first, c->path);
			}

			// Wait for a little bit so that the processMonitor detects the process termination
			// before we tell the listeners ... a bit of a hack
		
			Sleep(1000);
			/*
			printf("looking for process: %i\n", piProcessInfo.dwProcessId);
			HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, piProcessInfo.dwProcessId);
			if(hProc == NULL)
			{
				NotifyListeners(VISIT_FINISH, 0, url->first, c->path);
			} else {
				printf("Got process %i\n", GetProcessId(hProc));
				if(GetProcessId(hProc) == piProcessInfo.dwProcessId)
				{
					CloseHandle(hProc);
					NotifyListeners(VISIT_PROCESS_ERROR, 0, url->first, c->path);
				} else {
					NotifyListeners(VISIT_FINISH, 0, url->first, c->path);
				}
			}
			*/
			NotifyListeners(VISIT_FINISH, 0, url->first, c->path);
		}

		
finish:
		// Post event to pause all of the monitors
		NotifyListeners(VISIT_POSTFINISH, 0, url->first, c->path);
		visiting = false;
		//delete url;
		//delete client;
		//delete visit;
	}
}

BOOL CALLBACK Visitor::EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	DWORD wndPid;
	GetWindowThreadProcessId(hwnd, &wndPid);
	wchar_t szWindowTitle[1024];
	int length = GetWindowText(hwnd,szWindowTitle,1024);
	printf("enumwindowsproc callback: %i == %i\n", wndPid, (DWORD)lParam);
	if ( (wndPid == (DWORD)lParam) && (length != 0))
	{
		/*
		wchar_t szClassName[1024];
		
		if(GetClassName(hwnd,szClassName,1024))
		{
			printf("Got class name %ls\n", szClassName);
			HWND blah = FindWindowW(szClassName, NULL);
			PostMessage(blah, WM_CLOSE, 0, 0);
			//return false;
		}
		*/
		wchar_t szClassName[1024];
		GetClassName(hwnd,szClassName,1024);
		printf("Got class name %ls\n", szClassName);
		printf("sending wm_close\n");
		PostMessage(hwnd, WM_CLOSE, 0, 0);
		return false;
	}
    return true;
}

void
Visitor::OnReceiveEvent(vector<wstring> e)
{
	// Default client if none listed
	wstring findClient = L"iexplore";
	if(e.size() >= 2)
	{
		// visit::<url>
		wstring visiturl = e[1];
		int visitTime = 30;
		// visit::<url>::time || visit::<client>::<url>
		if(e.size() >= 3)
		{
			if(visitorClientsMap.find(e[1]) == visitorClientsMap.end())
			{
				visiturl = e[1];
				visitTime = _wtoi(e[2].c_str());
			} else {
				findClient = e[1];
				visiturl = e[2];
			}
		}
		// visit::<client>::<url>::<time>
		if(e.size() >= 4)
		{
			findClient = e[1];
			visiturl = e[2];
			visitTime = _wtoi(e[3].c_str());
		}
		stdext::hash_map<wstring, Client *>::iterator it;
		it = visitorClientsMap.find(findClient);
		if(it != visitorClientsMap.end())
		{
			Client_Pair client = *it;

			Url_Pair* url = new Url_Pair(visiturl, visitTime);
			Visit_Pair* visit = new Visit_Pair(client, url);
			toVisit.push(visit);
			SetEvent(hQueueNotEmpty);
		} else {
			printf("ERROR - Could not find client %ls path, url not queued for visitation\n", findClient.c_str());
		}
	}
}

void 
Visitor::AddVisitorListener(VisitorListener* vl)
{
	visitListeners.push_back(vl);
}

void
Visitor::RemoveVisitorListener(VisitorListener* vl)
{
	visitListeners.remove(vl);
}

void
Visitor::NotifyListeners(int eventType, DWORD minorEventCode, wstring url, wstring visitor)
{
	std::list<VisitorListener*>::iterator lit;
	
	// Inform all registered listeners of event
	for (lit=visitListeners.begin(); lit!= visitListeners.end(); lit++)
	{
		(*lit)->OnVisitEvent(eventType, minorEventCode, url, visitor);
	}
}

stdext::hash_map<wstring, Client*>*
Visitor::GetVisitorClients()
{
	return &visitorClientsMap;
}
