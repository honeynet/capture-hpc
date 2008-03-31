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
#include "Server.h"
#include "ServerSend.h"
#include "ServerReceive.h"

#include <boost/bind.hpp>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdlib.h>


Server::Server(const std::wstring& sAddress, int p)
{
	InitializeCriticalSection(&sendQueueLock);
	serverAddress = sAddress;
	port = p;
	connected = false;
	//serverSend = new ServerSend(this);
	serverReceive = new ServerReceive(this);
}

Server::~Server()
{
	//delete serverSend;
	delete serverReceive;
	WSACleanup();
	disconnectFromServer();
}


bool
Server::connectToServer(bool startSenderAndReciever)
{
	if(initialiseWinsock2())
	{
		char sport[6];
		size_t charsConverted;
		char szServerAddress[2048];
		struct addrinfo aiHints;
		struct addrinfo *aiList = NULL;	
			
		_itoa_s(port, sport, 6, 10);
		memset(&aiHints, 0, sizeof(aiHints));
		aiHints.ai_family = AF_INET;
		aiHints.ai_socktype = SOCK_STREAM;
		aiHints.ai_protocol = IPPROTO_TCP;
		DebugPrint(L"Connecting to: %ls:%s\n", serverAddress.c_str(), sport);
		wcstombs_s(&charsConverted, szServerAddress, 512, serverAddress.c_str(), serverAddress.length() + 1);
		if ((getaddrinfo(szServerAddress, sport, &aiHints, &aiList)) == 0)
		{
			struct addrinfo *ai = aiList;
			struct sockaddr_in *sa = (struct sockaddr_in *) ai->ai_addr;
		
			serverSocket = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
			
			int count = 0;
			while(count < 10) {
				if(connect(serverSocket, (struct sockaddr *)sa, static_cast<int>(ai->ai_addrlen) ) != SOCKET_ERROR )
				{
					u_long iMode = 1;
					//ioctlsocket(serverSocket, FIONBIO, &iMode);
					printf("Connected to server at %s\n", szServerAddress);
					setConnected(true);
					if(startSenderAndReciever)
					{
						serverReceive->start();
					}
					freeaddrinfo(aiList);
					return true;
				} else {
					printf("Could not connect to server\n");
					printf("\tSocket error: %i\n", WSAGetLastError());
					printf("Retrying...\n");
					Sleep(1000);
					count++;
				}
			}
			freeaddrinfo(aiList);
		}	
	}
	setConnected(false);
	return false;
}

void
Server::sendMessage(const std::wstring& message)
{
	if(isConnected())
	{

		int result = 0;
		int mbsize = WideCharToMultiByte(CP_UTF8, 0, message.c_str(), static_cast<int>(message.length()), NULL, 0, NULL, NULL);
		if (mbsize > 0)
		{
			char *szMessage = (char*)malloc(mbsize);
			int bytes = WideCharToMultiByte(CP_UTF8, 0, message.c_str(), static_cast<int>(message.length()), szMessage, mbsize, NULL, NULL);
			EnterCriticalSection(&sendQueueLock);
			result = send(serverSocket, szMessage, bytes, 0);	
			LeaveCriticalSection(&sendQueueLock);
			DebugPrint(L"Capture-Server-sendMessage: Allocated: %i, Converted: %i, Sent: %i\n", mbsize, bytes, result);
			if(result == SOCKET_ERROR)
			{
				DebugPrint(L"Capture-Server-sendMessage: Socket Error %i: %s\n", WSAGetLastError(), szMessage);
				setConnected(false);
			}
			free(szMessage);
		}	
	}
}

void
Server::sendData(const char* data, size_t length)
{
	if(isConnected())
	{
		int result = send(serverSocket, data, static_cast<int>(length), 0);
		if(result == SOCKET_ERROR)
		{
			DebugPrint(L"Capture-Server-sendData: Socket Error %i", WSAGetLastError());
			setConnected(false);
		}
	}
}

void
Server::sendElement(const Element& element)
{
	if(!isConnected())
	{
		return;
	}
	sendMessage(element.toString());
	/*
	EnterCriticalSection(&sendQueueLock);
	std::wstring xmlDocument = L"<";
	xmlDocument += element.getName();
	std::vector<Attribute>::const_iterator it;
	for(it = element.getAttributes().begin(); it != element.getAttributes().end(); it++)
	{
		xmlDocument += L" ";
		xmlDocument += (*it).getName();
		xmlDocument += L"=\"";
		xmlDocument += xml_escape((*it).getValue());
		xmlDocument += L"\"";
	}
	if(element.getData() != NULL)
	{
		xmlDocument += L">";
		sendMessage(xmlDocument);
		sendData(element.getData(), element.getDataSize());
		xmlDocument = L"</";
		xmlDocument += element.getName();
		xmlDocument += L">\r\n";
		sendMessage(xmlDocument);
	} else {
		xmlDocument += L"/>\r\n";
		sendMessage(xmlDocument);
	}
	LeaveCriticalSection(&sendQueueLock);
	*/
	
}

std::wstring
Server::xml_escape(const std::wstring& xml)
{
	std::wstring escaped = xml;
	for(unsigned int i = 0; i < xml.length(); i++)
	{
		if(escaped.at(i) == 0x0026) {
			escaped = escaped.replace(i, 1, L"&amp;");
		} else if(escaped.at(i) == 0x003C) {
			escaped = escaped.replace(i, 1, L"&lt;");
		} else if(escaped.at(i) == 0x003E) {
			escaped = escaped.replace(i, 1, L"&gt;");
		} else if(escaped.at(i) == 0x0022) {
			escaped = escaped.replace(i, 1, L"&quot;");
		} else if(escaped.at(i) == 0x0027) {
			escaped = escaped.replace(i, 1, L"&apos;");
		}
	}
	return escaped;
}

void
Server::sendXML(const std::wstring& elementName, const std::vector<Attribute>& vAttributes)
{
	if(!isConnected())
	{
		return;
	}
	//EnterCriticalSection(&sendQueueLock);
	std::wstring xmlDocument = L"<";
	xmlDocument += elementName;
	std::vector<Attribute>::const_iterator it;
	for(it = vAttributes.begin(); it != vAttributes.end(); it++)
	{
		xmlDocument += L" ";
		xmlDocument += it->getName();
		xmlDocument += L"=\"";
		xmlDocument += xml_escape(it->getValue());;
		xmlDocument += L"\"";
	}
	xmlDocument += L"/>\r\n";
	sendMessage(xmlDocument);
	//LeaveCriticalSection(&sendQueueLock);
}

void
Server::disconnectFromServer()
{
	closesocket(serverSocket);
	WSACleanup();
	if(isConnected())
	{
		setConnected(false);
	}
}

bool
Server::isConnected()
{
	return connected;
}

void
Server::setConnected(bool c)
{
	connected = c;
	signalSetConnected(connected);
}

boost::signals::connection 
Server::onConnectionStatusChanged(const signal_setConnected::slot_type& s)
{ 
	return signalSetConnected.connect(s); 
}

bool
Server::initialiseWinsock2()
{
	WSADATA wsaData;
	WORD version;
	int error;

	version = MAKEWORD( 2, 0 );

	error = WSAStartup( version, &wsaData );

	if ( error == 0 )
	{
		if ( LOBYTE( wsaData.wVersion ) == 2 ||
		 HIBYTE( wsaData.wVersion ) == 0 )
		{
			return true;
		} else {
			WSACleanup();
		}
	}
	return false;
}
