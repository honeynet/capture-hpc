#include "Precompiled.h"

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
		LOG(INFO, "ServerReceive::stop() stopping thread.\n");
		receiver->stop();
		DWORD dwWaitResult;
		dwWaitResult = receiver->wait(5000);
		switch (dwWaitResult) 
		{
        // All thread objects were signaled
        case WAIT_OBJECT_0: 
            LOG(INFO, "ServerReceive::stop() stopped receiver.\n");
			break;
		case WAIT_TIMEOUT:
			LOG(INFO, "ServerReceive::stop() stopping receiver timed out. Attempting to terminate.\n");
			receiver->terminate();
			LOG(INFO, "ServerReceive::stop() terminated sender.\n");
			break;
        // An error occurred
        default: 
            LOG(INFO, "ServerReceive stopping receiver failed (%d)\n", GetLastError());
		} 
	}
	running = false;
}

void
ServerReceive::run()
{
	while(running && server->isConnected())
	{
		char buffer[MAX_RECEIVE_BUFFER];
		std::string xmlDocument = "";
		int bytesRecv = 0;
		while(running && (bytesRecv = recv(server->serverSocket, buffer, MAX_RECEIVE_BUFFER, 0)) > 2)
		{
			LOG(INFO, "ServerReceive. Bytes received: %d\n",bytesRecv);
			for(int i = 0; i < bytesRecv; i++)
			{
				if(buffer[i] != '\0')
				{
					xmlDocument += buffer[i];
				} else {
					LOG(INFO, "Got: %s\n", xmlDocument.c_str());
					LOG(INFO, "ServerReceive: Received document - length: %i\n", xmlDocument.length());
					EventController::getInstance()->receiveServerEvent(xmlDocument.c_str());
					xmlDocument = "";
				}
			}
			Sleep(100);
		}

		if(bytesRecv<0) {
			LOG(INFO, "ServerReceive. Bytes received2: %d\n",bytesRecv);
			LOG(INFO, "ServerReceive. Recv failed: %d\n", WSAGetLastError());
			server->reconnect();
		}
	}
	running = false;
}
