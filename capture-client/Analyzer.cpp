/*
 *	PROJECT: Capture
 *	FILE: Analyzer.cpp
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
#include "Analyzer.h"

Analyzer::Analyzer(int argc, CHAR* argv[])
{
	char* server = NULL;
	
	if(argc > 1)
	{
		server = argv[1];
	} 
	if(argc == 4) {
		serverId = argv[2];
		vmId = argv[3];
	}
	Common* global = new Common();
	malicious = false;
	connected = false;

	serverConnection = new ServerConnection(global);

	visitor = new Visitor(global);
	visitor->AddVisitorListener(this);

	// Listen for events from the server
	wstring sEvent = L"connect";
	if(!global->serverEventDelegator->Register(this, sEvent))
	{
		printf("Analyzer not listening for connect events from server\n");
	}
	sEvent = L"ping";
	if(!global->serverEventDelegator->Register(this, sEvent))
	{
		printf("Analyzer not listening for ping events from server\n");
	}
	
	if(ObtainDriverLoadPrivilege() && ObtainDebugPrivilege())
	{
#ifndef LOCALTEST
		procMon = new ProcessMonitor(visitor);
		procMon->AddProcessListener(this);
		procMon->Start();
		
		regMon = new RegistryMonitor();
		regMon->AddRegistryListener(this);
		regMon->Start();
		fileMon = new FileMonitor();
		fileMon->AddFileListener(this);
		fileMon->Start();
#endif
		
	} else {
		printf("ERROR - Can't acquire SeLoadDriverPrivilege privilege, not loading kernel drivers\n");
	}
	
	if(server != NULL)
	{
		connected = serverConnection->Connect(server, 7070);
		if(connected == false)
		{
			printf("Could not connect to server: %ls:7070\n", server);
		}
	} else {
		printf("No server address found - Outputting events to console\n");

		//global->serverEventDelegator->ReceiveEvent(L"visit::opera::http://www.neowin.net::10\n");
		//global->serverEventDelegator->ReceiveEvent(L"visit::opera::http://www.neowin.net::10\n");
#ifndef LOCALTEST
		procMon->UnPause();
		fileMon->UnPause();
		regMon->UnPause();
#endif
		//AdobeAcrobatReaderC *mo = new AdobeAcrobatReaderC();
		//MicrosoftOfficeC *mo = new MicrosoftOfficeC();
		//mo->Open(L"word");
		//mo->Visit(L"Hello");
		//mo->Close();
	}
}

Analyzer::~Analyzer(void)
{
#ifndef LOCALTEST
	delete procMon;
	delete regMon;
	delete fileMon;
#endif
	delete visitor;
	delete serverConnection;
}

void 
Analyzer::OnProcessEvent(BOOLEAN bCreate, wstring time, wstring processParentPath, wstring processPath)
{
	// Process the event and create a generic event to be sent to the server
	printf("P: %i %ls -> %ls\n", bCreate, processParentPath.c_str(), processPath.c_str());
	malicious = true;
	wstring send = L"event::";
	send += time;
	send += L"::process::";
	if(bCreate)
	{
		send += L"Created::";
	} else {
		send += L"Terminated::";
	}
	send += processParentPath;
	send += L"::";
	send += processPath;
	send += L"\n";
	serverConnection->Send(send);

}

void
Analyzer::OnVisitEvent(int eventType, DWORD minorEventCode, wstring url, wstring visitor)
{
	
	if(eventType == VISIT_START)
	{
		printf("Visiting: %ls\n", url.c_str());
		wstring send = L"visiting::";
		send += url;
		send += L"\n";
		serverConnection->Send(send);
	} 
	else if(eventType == VISIT_FINISH)
	{
		printf("Visited: %ls\n", url.c_str());
		wstring send = L"visited::finished::";
		send += url;
		send += L"::";
		if(malicious)
			send += L"1\n";
		else
			send += L"0\n";
		serverConnection->Send(send);
		malicious = false;
	} 
	else if(eventType == VISIT_NETWORK_ERROR)
	{
		printf("Visited: NETWORK ERROR: %i %ls\n", minorEventCode,url.c_str());
		wstring send = L"visited::error::";
		send += url;
		send += L"::";
		send += L"network error";
		send += L"::";
		// Error code
		wchar_t szTemp[256];
		if(minorEventCode >= 0x800C0000) {
			_ltow_s(minorEventCode, szTemp, 256, 16);		
		} else {
			_ltow_s(minorEventCode, szTemp, 256, 10);
		}
		send += szTemp;
		send += L"\n";
		serverConnection->Send(send);
		malicious = false;
	} 
	else if(eventType == VISIT_TIMEOUT_ERROR)
	{
		printf("Visited: TIMEOUT ERROR %ls\n", url.c_str());
		wstring send = L"visited::error::";
		send += url;
		send += L"::";
		send += L"timeout error\n";
		serverConnection->Send(send);
		malicious = false;
	} else if(eventType == VISIT_PRESTART) {
#ifndef LOCALTEST
		procMon->UnPause();
		fileMon->UnPause();
		regMon->UnPause();
#endif
	} else if(eventType == VISIT_POSTFINISH) {
#ifndef LOCALTEST
		procMon->Pause();
		fileMon->Pause();
		regMon->Pause();
#endif
	} else if(eventType == VISIT_PROCESS_ERROR) {
		malicious = true;
		wstring send;
		printf("P: 0 Process terminated adbruptly during visitation -> %ls\n", visitor.c_str());
		SYSTEMTIME systemTime;
		GetLocalTime(&systemTime);

		wstring time = LocalTimeToWString(systemTime);
		send = L"event::";
		send += time;
		send += L"::process::Terminated Adbruptly::Capture";
		send += L"::";
		send += visitor;
		send += L"\n";
		serverConnection->Send(send);
	}
}

void 
Analyzer::OnRegistryEvent(wstring eventType, wstring time, wstring processFullPath, wstring registryPath)
{
	malicious = true;
	printf("R: %ls %ls -> %ls\n", eventType.c_str(), processFullPath.c_str(), registryPath.c_str());
	wstring send = L"event::";
	send += time;
	send += L"::registry::";
	send += eventType;
	send += L"::";
	send += processFullPath;
	send += L"::";
	send += registryPath;
	send += L"\n";

	serverConnection->Send(send);
}
void 
Analyzer::OnFileEvent(wstring eventType, wstring time, wstring processFullPath, wstring filePath)
{
	malicious = true;
	printf("F: %ls %ls -> %ls\n", eventType.c_str(), processFullPath.c_str(), filePath.c_str());
	wstring send = L"event::";
	send += time;
	send += L"::file::";
	send += eventType;
	send += L"::";
	send += processFullPath;
	send += L"::";
	send += filePath;
	send += L"\n";
	serverConnection->Send(send);
}

void
Analyzer::OnReceiveEvent(vector<wstring> e)
{
	if(e[0] == L"connect")
	{
		if(serverId != NULL && vmId != NULL)
		{
			size_t convertedChars = 0;
			size_t origsize = strlen(serverId) + 1;
			const size_t newsize = 100;
				
			wchar_t wServerId[newsize];
			wchar_t wVmId[newsize];
			mbstowcs_s(&convertedChars, wServerId, origsize, serverId, _TRUNCATE);
			mbstowcs_s(&convertedChars, wVmId, origsize, vmId, _TRUNCATE);
			wstring send = L"connect::";
			send += wServerId;
			send += L"::";
			send += wVmId;
			send += L"\n";
			printf("Sending connect: %ls\n", send.c_str());
			serverConnection->Send(send);
			stdext::hash_map<wstring, Client *>::iterator it;
			for (it=visitor->GetVisitorClients()->begin(); it!= visitor->GetVisitorClients()->end(); it++)
			{
				Client* c = it->second;
				if(c->clientProductName != L"") {
					send = L"client::";
					send += c->name;
					send += L"::";
					send += c->clientProductName;
					send += L"::";
					send += c->path;
					send += L"::";
					send += c->clientVersion;
					send += L"\n";
					//printf("Sending client: %ls\n", send.c_str());
					serverConnection->Send(send);
				}
			}
		}
	} else if(e[0] == L"ping") {
		printf("got ping\n");
		wstring send = L"pong::client\n";
		serverConnection->Send(send);
	}
}

bool
Analyzer::ObtainDriverLoadPrivilege()
{
	HANDLE hToken;
	if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken))
	{
		TOKEN_PRIVILEGES tp;
		LUID luid;
		LPCTSTR lpszPrivilege = L"SeLoadDriverPrivilege";

		if ( !LookupPrivilegeValue( 
				NULL,            // lookup privilege on local system
				lpszPrivilege,   // privilege to lookup 
				&luid ) )        // receives LUID of privilege
		{
			printf("LookupPrivilegeValue error: %u\n", GetLastError() ); 
			return false; 
		}

		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		// Enable the privilege or disable all privileges.
		if ( !AdjustTokenPrivileges(
			hToken, 
			FALSE, 
			&tp, 
			sizeof(TOKEN_PRIVILEGES), 
			(PTOKEN_PRIVILEGES) NULL, 
			(PDWORD) NULL) )
		{ 
		  printf("AdjustTokenPrivileges error: %u\n", GetLastError() );
		  return false; 
		} 

		if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
		{	
			printf("The token does not have the specified privilege. \n");
			return false;
		} 
	}
	CloseHandle(hToken);
	return true;

}

bool
Analyzer::ObtainDebugPrivilege()
{
	HANDLE hToken;
	if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken))
	{
		TOKEN_PRIVILEGES tp;
		LUID luid;
		LPCTSTR lpszPrivilege = L"SeDebugPrivilege";

		if ( !LookupPrivilegeValue( 
				NULL,            // lookup privilege on local system
				lpszPrivilege,   // privilege to lookup 
				&luid ) )        // receives LUID of privilege
		{
			printf("LookupPrivilegeValue error: %u\n", GetLastError() ); 
			return false; 
		}

		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		// Enable the privilege or disable all privileges.
		if ( !AdjustTokenPrivileges(
			hToken, 
			FALSE, 
			&tp, 
			sizeof(TOKEN_PRIVILEGES), 
			(PTOKEN_PRIVILEGES) NULL, 
			(PDWORD) NULL) )
		{ 
		  printf("AdjustTokenPrivileges error: %u\n", GetLastError() );
		  return false; 
		} 

		if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
		{	
			printf("The token does not have the specified privilege. \n");
			return false;
		} 
	}
	CloseHandle(hToken);
	return true;
}