/*
 *	PROJECT: Capture
 *	FILE: ServerConnection.cpp
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
#include "ServerConnection.h"

ServerConnection::ServerConnection(Common* g)
{
	global = g;
	InitializeCriticalSection(&sendQueueLock);
	connected = false;
	wstring sEvent = L"visit";
	global->serverEventDelegator->AddEventType(sEvent);
	wstring sEvent2 = L"connect";
	global->serverEventDelegator->AddEventType(sEvent2);
	wstring sPing = L"ping";
	global->serverEventDelegator->AddEventType(sPing);
	sendQueue = new queue<wstring>();
	//hSendQueue = CreateEvent(NULL, FALSE, FALSE, NULL);
}

ServerConnection::~ServerConnection(void)
{
	closesocket(client);
	WSACleanup();
}

bool
ServerConnection::InitWinSock2()
{
	WSADATA wsaData;
	WORD version;
	int error;

	version = MAKEWORD( 2, 0 );

	error = WSAStartup( version, &wsaData );

	/* check for error */
	if ( error != 0 )
	{
		/* error occured */
		return false;
	}

	/* check for correct version */
	if ( LOBYTE( wsaData.wVersion ) != 2 ||
		 HIBYTE( wsaData.wVersion ) != 0 )
	{
		/* incorrect WinSock version */
		WSACleanup();
		return false;
	}
	return true;
}

bool
ServerConnection::Connect(char *address, int port)
{
	if(!InitWinSock2())
	{
		return false;
	}

	struct addrinfo aiHints;
	struct addrinfo *aiList = NULL;

	//--------------------------------
	// Setup the hints address info structure
	// which is passed to the getaddrinfo() function
	memset(&aiHints, 0, sizeof(aiHints));
	aiHints.ai_family = AF_INET;
	aiHints.ai_socktype = SOCK_STREAM;
	aiHints.ai_protocol = IPPROTO_TCP;

	char *sport = "7070";
	if ((getaddrinfo(address, sport, &aiHints, &aiList)) != 0) {
		printf("getaddrinfo() failed.\n");
	}

	if(aiList == NULL) 
	{
		printf("Could not find host\n");
		return false;
	}
	struct addrinfo *ai = aiList;
	struct sockaddr_in *sa = (struct sockaddr_in *) ai->ai_addr;
	client = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	int turn = 0;
	bool tconnected = false;
	while((turn < 10) && !tconnected)
	{
		turn++;
		if ( connect( client, (struct sockaddr *)sa, static_cast<int>(ai->ai_addrlen) ) == SOCKET_ERROR )
		{
			printf("WARNING Could not connect to server at %s:%i - %i - (%i/10)\n", inet_ntoa(sa->sin_addr), port, WSAGetLastError(), turn);
			Sleep(2000);
		} else {
			printf("Connected successfully to server at %s:%i\n",inet_ntoa(sa->sin_addr), port);
			tconnected = true;
		}
	}
	if(!tconnected)
	{
		freeaddrinfo(aiList);
		printf("ERROR Could not connect to server at %s:%i\n", inet_ntoa(sa->sin_addr), port);
		return false;
	}

	u_long iMode = 0;
	ioctlsocket(client, FIONBIO, &iMode);
	freeaddrinfo(aiList);
	
	ServerSend* serverSend = new ServerSend(this);
	ServerReceive* serverReceive = new ServerReceive(this);

	Thread* send = new Thread(serverSend);
	Thread* receive = new Thread(serverReceive);

	send->Start();
	receive->Start();
	connected = true;
	return true;

}


void
ServerConnection::Send(wstring msg)
{	
	if(connected) {
		//printf("adding\n");
		EnterCriticalSection(&sendQueueLock);
		sendQueue->push(msg);
		LeaveCriticalSection(&sendQueueLock);
		//SetEvent(hSendQueue);
	}
}

ServerSend::ServerSend(ServerConnection* sc)
{
	serverConnection = sc;
}

void
ServerSend::Run()
{
	while(serverConnection->connected)
	{		
		if(!serverConnection->sendQueue->empty())
		{
			EnterCriticalSection(&serverConnection->sendQueueLock);
			wstring msg = serverConnection->sendQueue->front();
			serverConnection->sendQueue->pop();
			LeaveCriticalSection(&serverConnection->sendQueueLock);

			size_t maxSize = 2048;
			char *szMsg = (char*)malloc(maxSize);

			size_t charsConverted;
			wcstombs_s(&charsConverted, szMsg, maxSize, msg.c_str(), msg.length() + 1);

			if(send(serverConnection->client, szMsg, charsConverted, 0) == SOCKET_ERROR)
			{
				printf("send() failed with error: %d\n", WSAGetLastError());
				if(WSAGetLastError() == WSAENOTCONN)
				{
					printf( "Disconnected from server\n");
					serverConnection->connected = false;
				}
			}
			free(szMsg);
		} else {
			Sleep(100);
		}
	}
}

ServerReceive::ServerReceive(ServerConnection* sc)
{
	serverConnection = sc;
}

void
ServerReceive::Run()
{
	int bytesRecv = 0;
	while( bytesRecv != SOCKET_ERROR && serverConnection->connected)
	{
		char buff[1024];
		ZeroMemory(&buff, sizeof(buff));
		bytesRecv = recv(serverConnection->client, buff, 1024, 0);
		if(bytesRecv == 0)
		{
			printf( "Disconnected from server\n");
			serverConnection->connected = false;
			break;
		}
		
		size_t origsize = strlen(buff) + 1;
		const size_t newsize = 1024;
		size_t convertedChars = 0;
		wchar_t wcstring[newsize];
		mbstowcs_s(&convertedChars, wcstring, origsize, buff, _TRUNCATE);
		printf("Received: %ls\n", wcstring);
		// Received something so pass it onto the event delegator
		serverConnection->global->serverEventDelegator->ReceiveEvent(wcstring);
	}
}


