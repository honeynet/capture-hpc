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
		receiver->exit();
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
			for(int i = 0; i < bytesRecv; i++)
			{
				if(buffer[i] != '\0')
				{
					xmlDocument += buffer[i];
				} else {
					LOG(INFO, "Server: received %i %s", xmlDocument.length(), xmlDocument.c_str());
					EventController::getInstance()->receiveServerEvent(xmlDocument.c_str());
					xmlDocument = "";
				}
			}
			Sleep(100);
		}

		if(bytesRecv<0) {
			LOG(ERR, "Server: receive error - %d - %d", bytesRecv, WSAGetLastError());
			server->reconnect();
		}
	}
	running = false;
}
