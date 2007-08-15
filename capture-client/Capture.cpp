/*
 *	PROJECT: Capture
 *	FILE: Capture.cpp
 *	AUTHORS: Ramon Steenson (rsteenson@gmail.com) & Christian Seifert (christian.seifert@gmail.com)
 *
 *	Supported by Victoria University of Wellington and the New Zealand Honeynet Alliance
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
#include "stdafx.h"
#include "Visitor.h"
#include "ServerConnection.h"
#include "ProcessMonitor.h"
#include "Analyzer.h"
#include "lib\TestHarness.h"

#ifndef UNITTEST
int Common::count = 0;
#endif

#ifdef UNITTEST

int Common::count = 0;
Common* global = new Common();
#include "lib\TestHarness.h"

class TestListener : public ServerListener
{
public:
	void OnReceiveEvent(vector<wstring> e)
	{
		//printf("got recent event\n");
		msg = e[0];
	}
	wstring msg;
};
#define DEFAULT_BUFLEN 12
#define DEFAULT_PORT "7070"
class TestServer : public IRunnable
{
public:
	void Run()
	{
		WSADATA wsaData;
		SOCKET ListenSocket = INVALID_SOCKET,
			ClientSocket = INVALID_SOCKET;
		struct addrinfo *result = NULL,
			hints;
		char recvbuf[DEFAULT_BUFLEN];
		int  iResult,
			recvbuflen = DEFAULT_BUFLEN;
    

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return;
    }

    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return;
    }
    closesocket(ListenSocket);
	ZeroMemory(&recvbuf, sizeof(recvbuf));
	string s = "visit::HelloClient\n";
	send(ClientSocket,s.c_str(),s.length(),0);
    // Receive until the peer shuts down the connection
    do {
	
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			size_t origsize = strlen(recvbuf) + 1;
			const size_t newsize = 100;
			size_t convertedChars = 0;
			
			mbstowcs_s(&convertedChars, received, origsize, recvbuf, _TRUNCATE);
			//printf("received: %ls -- %i\n", received, convertedChars);
			
			//wstring test = wcstring;
			//printf("received2: \n");
			//received = wcstring;
			//received = received.substr(0,recvbuflen);
			
		//} else if (iResult == 0) {

		//} else  {
			Sleep(1000);
            closesocket(ClientSocket);
            WSACleanup();
            return;
        }

		} while (iResult > 0);
	}
	wchar_t received[100];
};



TEST (ServerConnection,Receive)
{
	printf("Start ServerConnection,Receive\n");

	TestListener* testListener = new TestListener();
	global->serverEventDelegator->Register(testListener, L"visit");

		TestServer* ts = new TestServer();
	Thread* t = new Thread(ts);
	t->Start();
	Sleep(2000);

	ServerConnection* sc = new ServerConnection(global);
	bool connected = sc->Connect("127.0.0.1",7070); 
	CHECK(connected == true);
	Sleep(2000);
	bool received = false;
	if(testListener->msg.size() > 0) {
		received = true;
	}
	CHECK(received == true);
}



#include "VisitorListener.h"
/* Subclass of visitor so that checks can be made to insure
 * that it correctly received information and visits a url */
class TestVisitor : public Visitor, VisitorListener
{
public:
	int count;
	bool visited;
	TestVisitor(Common* g) : Visitor(g)
	{
		Visitor::AddVisitorListener(this);
		count = 0;
		visited = false;
	}

	bool checkReceived()
	{
		if(toVisit.size() == 1)
			return true;
		return false;
	}
	void OnVisitEvent(int eventType, wstring url, wstring visitor)
	{
		if(count == 0)
		{
			if(eventType == VISIT_START)
			{
				if(url == L"http://www.google.com")
					count++;
			}
		} else if(count == 1)
		{
			if(eventType == VISIT_FINISH)
			{
				if(url == L"http://www.google.com")
					visited = true;
			}	
		}
	}
	~TestVisitor()
	{
	}

};


TEST (Visitor,VisitURL)
{
	printf("Start Visitor,VisitURL\n");
	TestVisitor* visitor = new TestVisitor(global);
	vector<wstring> v;
	v.push_back(L"visit");
	v.push_back(L"http://www.google.com");
	visitor->OnReceiveEvent(v);
	bool VisitorReceivedVisitEvent = visitor->checkReceived();
	// Check if OnReceivedEvent gets the correct information
	// Makes sure it is correctly registered for visit events
	CHECK(VisitorReceivedVisitEvent == true);
	
	printf("Waiting 37 seconds for visit to complete\n");
	Sleep(37000);

	// Make sure the visitor actually visits the specified url
	CHECK(visitor->visited == true);
	
	delete visitor;
}

class TestProcess : public ProcessListener
{
public:
	void OnProcessEvent(BOOLEAN bCreate, wstring processParentPath, wstring processPath)
	{
		if(bCreate)
		{	
			if(processPath == L"C:\\WINDOWS\\system32\\calc.exe")
				processCreated = true;
		} else {
			if(processPath == L"C:\\WINDOWS\\system32\\calc.exe")
				processTerminated = true;
		}
	}
	bool processCreated;
	bool processTerminated;
};

TEST (ProcessMonitor, CreateAndTerminateProcess)
{
	printf("Start ProcessMonitor, CreateAndTerminateProcess\n");
	STARTUPINFO siStartupInfo;
	PROCESS_INFORMATION piProcessInfo;
	memset(&siStartupInfo, 0, sizeof(siStartupInfo));
	memset(&piProcessInfo, 0, sizeof(piProcessInfo));
	siStartupInfo.cb = sizeof(siStartupInfo);

	TestProcess* p = new TestProcess();
	ProcessMonitor* procMon = new ProcessMonitor();
	procMon->AddProcessListener(p);
	procMon->Start();
	procMon->UnPause();
	Sleep(2000);
	wchar_t blah[256];
	ZeroMemory(&blah, sizeof(blah));
	_tcscat_s(blah, 256, L"\"C:\\WINDOWS\\System32\\calc.exe\"");
	LPTSTR szCmdline=_tcsdup(blah);
	CreateProcess(NULL,szCmdline, 0, 0, false,
		CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo,
		&piProcessInfo);
	Sleep(5000);
	CHECK(p->processCreated == true);
	
	TerminateProcess(piProcessInfo.hProcess, 0);

	Sleep(2000);

	CHECK(p->processTerminated == true);
	
	procMon->Stop();
	delete procMon;
	delete p;
}

#include "Monitor.h"

TEST(Permission, CheckSimplePermissionIsWorking)
{
	printf("Start Permission, CheckSimplePermissionIsWorking\n");
	Permission* p = new Permission();
	boost::wregex s(L"capture", boost::regex::icase);
	boost::wregex e(L"test", boost::regex::icase);
	p->permissions.push_back(e);
	p->subjects.push_back(s);

	bool CheckIfSimpleRegexSucceeds = p->Check(L"capture", L"test");
	CHECK(CheckIfSimpleRegexSucceeds == true);
	
	bool CheckIfSimpleRegexFails = p->Check(L"capture",L"hello");
	CHECK(CheckIfSimpleRegexFails == false);
	delete p;
}

TEST(Permission, CheckComplexPermissionIsWorking)
{
	printf("Start Permission, CheckComplexPermissionIsWorking\n");
	Permission* p = new Permission();
	boost::wregex s(L"capture", boost::regex::icase);
	boost::wregex e(L"test\\\\.*\\\\hello", boost::regex::icase);
	p->permissions.push_back(e);
	p->subjects.push_back(s);

	bool CheckIfComplexRegexSucceeds = p->Check(L"capture", L"test\\blah\\hello");
	CHECK(CheckIfComplexRegexSucceeds == true);
	
	bool CheckIfComplexRegexFails = p->Check(L"capture", L"test\\blah\\fail");
	CHECK(CheckIfComplexRegexFails == false);
	delete p;
}

TEST(Monitor, CorrectlyLoadExclusionList)
{
	// Dont need to be implemented
}

TEST(Monitor, CorrectlyExcludeEventFromBeingMalicious)
{
	// Dont need to be implemented
}

#include "RegistryMonitor.h"

class TestReg : public RegistryListener
{
public:
	void OnRegistryEvent(wstring eventType, wstring processFullPath, wstring registryPath)
	{
		wchar_t szTemp[1024];
		GetCurrentDirectory(1024, szTemp);
		wcscat_s(szTemp, 1024, L"\\Capture.exe");
		wstring path = szTemp;
		if(eventType == L"OpenKey")
			if(processFullPath == path)
				if(registryPath == L"HKCU\\Software\\Microsoft")
					found = true;
	}

	bool found;
};

TEST(RegistryMonitor, CorrectlyDetectRegistryEvent)
{
	printf("Start RegistryMonitor::CorrectlyDetectRegistryEvent\n");
	TestReg* tr = new TestReg();
	tr->found = false;
	RegistryMonitor* regmon = new RegistryMonitor();
	regmon->AddRegistryListener(tr);
	regmon->permissionMap.clear();
	regmon->Start();
	regmon->UnPause();
   HKEY hTestKey;

   if( RegOpenKeyExA( HKEY_CURRENT_USER,
        "SOFTWARE\\Microsoft",
        0,
        KEY_READ,
        &hTestKey) == ERROR_SUCCESS
      )
   {
	   Sleep(10000);
	   RegCloseKey(hTestKey);
   }
   regmon->Stop();
   delete regmon;
   bool CheckIfRegistryMonitorGotOpenEvent = tr->found;
   CHECK(CheckIfRegistryMonitorGotOpenEvent == true);
   delete tr;
}

#include "FileMonitor.h"

class TestFile : public FileListener
{
public:
	void OnFileEvent(wstring eventType, wstring processFullPath, wstring filePath)
	{
				wchar_t szTemp[1024];
		GetCurrentDirectory(1024, szTemp);
		wcscat_s(szTemp, 1024, L"\\Capture.exe");
		wstring path = szTemp;
		if(eventType == L"Write")
			if(processFullPath == path)
				if(filePath == L"C:\\test.txt")
					found = true;
	}

	bool found;
};

TEST(FileMonitor, CorrectlyDetectFileEvent)
{
	printf("Start FileMonitor::CorrectlyDetectFileEvent\n");
	TestFile* tf = new TestFile();
	tf->found = false;
	FileMonitor* filemon = new FileMonitor();
	filemon->AddFileListener(tf);
	filemon->permissionMap.clear();
	filemon->Start();
	filemon->UnPause();

	ofstream file ("C:\\test.txt");
	char* text = "HelloWorld";
	file.write(text, sizeof(text));

	file.close();
	Sleep(5000);

	delete filemon;
	remove( "C:\\test.txt" );
	bool CheckIfFileMonitorGotFileEvent = tf->found;

	CHECK(CheckIfFileMonitorGotFileEvent == TRUE);
	
}
TEST (ServerConnection,Send)
{
	printf("Start ServerConnection::Send\n");
	TestServer* ts = new TestServer();
	Thread* t = new Thread(ts);
	t->Start();
	Sleep(2000);
	ServerConnection* sc = new ServerConnection(global);
	bool connected = sc->Connect("127.0.0.1",7070);
	CHECK(connected == true);
	wstring hello = L"HelloWorld\n";
	sc->Send(hello);

	Sleep(1000);

	// Check that the server received the string HelloServer
	bool ServerReceivedMessage = false;
	printf("Got: %ls\n", ts->received); 
	if(ts->received == hello) {
		ServerReceivedMessage = true;
		printf("Got: YAY IT WORKS\n"); 
	}
	//	delete ts;
	//delete sc;
	//delete t;
	t->Stop();
	//delete t;
	//delete sc;
	//delete ts;
	//printf("Got: DELETEd\n"); 
	CHECK(ServerReceivedMessage == true);
	

	//delete t;

}

#endif





/*
	File: Capture.cpp

	Main entry point into Capture
*/
int main(int argc, CHAR* argv[])
{
	#ifdef UNITTEST
		TestResult tr;
		TestRegistry::runAllTests(tr);
		getchar();
	#else

	Analyzer* a;
	a = new Analyzer(argc, argv);
	getchar();
	delete a;
		//delete server;
		
		//delete proc;
		//delete global;
	#endif
	return 0;
}

