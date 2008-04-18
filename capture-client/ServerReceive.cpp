#include "ServerReceive.h"
#include "Server.h"
#include "EventController.h"
#include <winsock2.h>

ServerReceive::ServerReceive(Server* s)
{
	server = s;
	running = false;

}

ServerReceive::~ServerReceive(void)
{
	stop();
}

void
ServerReceive::start()
{
	if(!running)
	{
		receiver = new Thread(this);
		receiver->start("ServerReceive");
		running = true;
	}
}

void
ServerReceive::stop()
{
	if(running)
	{
		DebugPrint(L"ServerReceive::stop() stopping thread.\n");
		receiver->stop();
		DWORD dwWaitResult;
		dwWaitResult = receiver->wait(5000);
		switch (dwWaitResult) 
		{
        // All thread objects were signaled
        case WAIT_OBJECT_0: 
            DebugPrint(L"ServerReceive::stop() stopped receiver.\n");
			break;
		case WAIT_TIMEOUT:
			DebugPrint(L"ServerReceive::stop() stopping receiver timed out. Attempting to terminate.\n");
			receiver->terminate();
			DebugPrint(L"ServerReceive::stop() terminated sender.\n");
			break;
        // An error occurred
        default: 
            printf("ServerReceive stopping receiver failed (%d)\n", GetLastError());
		} 
	}
	running = false;
}

void
ServerReceive::run()
{
	DebugPrintTrace(L"ServerReceive::run() start\n");
	while(!receiver->shouldStop() && server->isConnected())
	{
		char buffer[MAX_RECEIVE_BUFFER];
		std::string xmlDocument = "";
		int bytesRecv = 0;
		while(!receiver->shouldStop() && (bytesRecv = recv(server->serverSocket, buffer, MAX_RECEIVE_BUFFER, 0)) > 2)
		{
			printf("ServerReceive. Bytes received: %d\n",bytesRecv);
			for(int i = 0; i < bytesRecv; i++)
			{
				if(buffer[i] != '\0')
				{
					xmlDocument += buffer[i];
				} else {
					printf("Got: %s\n", xmlDocument.c_str());
					DebugPrint(L"ServerReceive: Received document - length: %i\n", xmlDocument.length());
					EventController::getInstance()->receiveServerEvent(xmlDocument.c_str());
					xmlDocument = "";
				}
			}
			Sleep(100);
		}

		if(bytesRecv<0) {
			printf("ServerReceive. Bytes received2: %d\n",bytesRecv);
			printf("ServerReceive. Recv failed: %d\n", WSAGetLastError());
			server->reconnect();
		}
	}
	running = false;
	DebugPrintTrace(L"ServerReceive::run() end\n");
}
