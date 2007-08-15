/*
 *	PROJECT: Capture
 *	FILE: ServerConnection.h
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
#pragma once
#include <windows.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <process.h>
#include <string>
#include <queue>
#include <vector>
#include "Thread.h"

using namespace std;

/*
   Class: ServerConnection

   The server connection is responsible for connecting up to the server, and sending and receiving messages

   
   It is based on the WinSock2 windows library and starts up 2 threads (<ServerSend> and <ServerReceive>) upon successful connection.
   When a connection has been established and an error occurs on the socket, new messages will not be queued nor will a connection
   try and be re-established. It is up to the server to detect and handle an error such as this. If no server address is specified the
   client will run with no logging support and instead will just pipe the message to STDIN

*/
class ServerConnection
{
	friend class ServerSend;
	friend class ServerReceive;
public:
	ServerConnection(Common* g);
	~ServerConnection(void);

	/*
		Function: Connect

		Connects to the Capture server at the specified address on a particular port
	*/
	bool Connect(char* address, int port);
	
	/*
		Function: Send

		Queues a message to be sent via the <ServerSend> thread that is started upon successful connection
	*/
	void Send(wstring msg);
	
	
protected:
	/*
		Function: InitWinSock2

		Initialises and checks the WinSock2 library in Windows
	*/
	bool InitWinSock2();

	/* 
		Variable: connected

		Specifies whether or not the client is connected to a server
	*/
	bool connected;
	
	/* 
		Variable: client

		Socket to the connected server
	*/
	SOCKET client;

	Common* global;

	/* 
		Variable: sendQueueLock

		Lock to protect access to the <sendQueue> so that only one thread may access it at a time
	*/
	CRITICAL_SECTION sendQueueLock;

	/* 
		Variable: sendQueue

		Queue that contains messages to be sent to the server
	*/
	queue<wstring>* sendQueue;


};

/*
   Class: ServerSend

   Server send thread which is run after a connection to the server has been established. The role of the thread is
   to watch the <sendQueue> and send messages to the server when messages are added to it
*/
class ServerSend : public IRunnable
{
public:
	ServerSend(ServerConnection* sc);

	~ServerSend();

	/*
		Function: Run

		Runnable thread which waits for data to be queued and sends it to the server
	*/
	virtual void Run();
private:
	ServerConnection* serverConnection;
};

/*
	Class: ServerReceive

	Thread which is responsible for received messages from the server and performing particular actions based on the message type
*/
class ServerReceive : public IRunnable
{
public:
	ServerReceive(ServerConnection* sc);

	~ServerReceive();

	/*
		Function: Run

		Runnable thread which receives messages from the server and passes it to the <ServerEventDelegator> where the message is handled
	*/
	virtual void Run();
private:
	ServerConnection* serverConnection;
};


