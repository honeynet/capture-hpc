#include "Updater.h"
#include "Server.h"
#include "EventController.h"
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

Updater::Updater(Server* s)
{
	server = s;

	onReceiveUpdateEventConnection = EventController::getInstance()->connect_onServerEvent(L"update", boost::bind(&Updater::onReceiveUpdateEvent, this, _1));
	onReceiveUpdatePartEventConnection = EventController::getInstance()->connect_onServerEvent(L"update-part", boost::bind(&Updater::onReceiveUpdatePartEvent, this, _1));
	onReceiveUpdateFinishedEventConnection = EventController::getInstance()->connect_onServerEvent(L"update-finished", boost::bind(&Updater::onReceiveUpdateFinishedEvent, this, _1));
}

Updater::~Updater(void)
{
	if(fileAllocated)
	{
		free(fileBuffer);
	}
	onReceiveUpdateEventConnection.disconnect();
	onReceiveUpdatePartEventConnection.disconnect();
	onReceiveUpdateFinishedEventConnection.disconnect();
}


void
Updater::onReceiveUpdateEvent(const Element& element)
{
	Element send_element = element;
	if(!fileAllocated)
	{
		std::vector<Attribute>::const_iterator it;
		for(it = element.getAttributes().begin(); it != element.getAttributes().end(); it++)
		{
			if((*it).getName() == L"file-size") {
				fileSize = boost::lexical_cast<int>((*it).getValue());
			} else if((*it).getName() == L"file-name") {
				fileName = (*it).getValue();
			}
		}
		fileBuffer = (char*)malloc(fileSize);
		fileAllocated = true;
		send_element.setName(L"update-accept");
		server->sendElement(send_element);
	} else {
		printf("Updater: ERROR - Can only download one update at a time\n");
		send_element.setName(L"update-reject");
		server->sendElement(send_element);
	}
}

void
Updater::onReceiveUpdatePartEvent(const Element& element)
{	
	int start = 0;
	int end = 0;
	int partSize = 0;
	Element send_element = element;	
	std::vector<Attribute>::const_iterator it;

	for(it = element.getAttributes().begin(); it != element.getAttributes().end(); it++)
	{
		if((*it).getName() == L"file-part-start") {
			start = boost::lexical_cast<int>((*it).getValue());
		} else if((*it).getName() == L"file-part-end") {
			end = boost::lexical_cast<int>((*it).getValue());
		}
	}

	if(fileAllocated)
	{
		partSize = end - start;

		// TODO Check the checksum of the part is correct

		char* decodedData = Base64::decode(send_element.getData());

		memcpy((void*)fileBuffer[start], decodedData, partSize);

		free(decodedData);
	} else {
		printf("Updater: ERROR - File not allocated: %08x\n", GetLastError());
		send_element.setName(L"update-error");
		server->sendElement(send_element);
	}
}

void
Updater::onReceiveUpdateFinishedEvent(const Element& element)
{
	HANDLE hFile;
	int offset = 0;
	Element send_element = element;
	hFile = CreateFile(fileName.c_str(),    // file to open
                   GENERIC_WRITE,          // open for reading
                   FILE_SHARE_READ,       // share for reading
                   NULL,                  // default security
                   CREATE_ALWAYS,         // existing file only
                   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, // normal file
                   NULL);                 // no attr. template
	if(hFile ==  INVALID_HANDLE_VALUE)
	{
		printf("Updater: ERROR - Could not open file: %08x\n", GetLastError());
		send_element.setName(L"update-error");
		server->sendElement(send_element);
	} else {
		DWORD bytesWritten = 0;
		WriteFile(hFile, fileBuffer, fileSize, &bytesWritten, NULL);
		send_element.setName(L"update-ok");
		server->sendElement(send_element);
		fileAllocated = false;
		free(fileBuffer);
	}
}