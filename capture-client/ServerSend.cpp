#include "ServerSend.h"
#include "Server.h"


ServerSend::ServerSend(Server* s)
{
	server = s;
	running = false;
}

ServerSend::~ServerSend(void)
{
	stop();
}

void
ServerSend::start()
{
	if(!running)
	{
		sender = new Thread(this);
		sender->start("ServerSend");
		running = true;
	}
}

void
ServerSend::stop()
{
	if(running)
	{
		sender->stop();
	}
	running = false;
}

void
ServerSend::run()
{
	try {
		while(server->isConnected())
		{	
			EnterCriticalSection(&server->sendQueueLock);
			if(!server->sendQueue.empty())
			{
				int result = 0;
				std::wstring msg = server->sendQueue.front();
				server->sendQueue.pop_back();
				LeaveCriticalSection(&server->sendQueueLock);
				size_t allocateSize = (msg.length() + 1)*sizeof(wchar_t);
				char *szMessage = (char*)malloc(allocateSize);
				size_t charsConverted;		
				wcstombs_s(&charsConverted, szMessage, allocateSize, msg.c_str(), msg.length() + 1);
				
				result = send(server->serverSocket, szMessage, static_cast<int>(charsConverted), 0);
				free(szMessage);
				DebugPrint(L"Capture-ServerSend: Allocated: %i, Converted: %i, Sent: %i", allocateSize, charsConverted, result);
				if(result == SOCKET_ERROR)
				{
					DebugPrint(L"Capture-ServerSend: Socket Error %i: %s", WSAGetLastError(), szMessage);
					if(WSAGetLastError() == WSAENOTCONN)
					{
						server->setConnected(false);
					}
				}
			} else {
				LeaveCriticalSection(&server->sendQueueLock);
				Sleep(100);
			}
		}
		
		running = false;
	} catch (...) {
		printf("ServerReceive::run exception\n");	
		throw;
	}
}