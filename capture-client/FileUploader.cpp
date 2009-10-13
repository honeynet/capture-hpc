#include "Precompiled.h"

#include "FileUploader.h"
#include "EventController.h"
#include "Server.h"

FileUploader::FileUploader(Server& s) : server(s)
{
	hFileAcknowledged = CreateEvent(NULL, FALSE, FALSE, NULL);
	fileAccepted = false;
	fileOpened = false;
	busy = false;

	onReceiveFileOkEventConnection = EventController::getInstance()->connect_onServerEvent(L"file-ok", boost::bind(&FileUploader::onReceiveFileOkEvent, this, _1));
	onReceiveFileErrorEventConnection = EventController::getInstance()->connect_onServerEvent(L"file-error", boost::bind(&FileUploader::onReceiveFileErrorEvent, this, _1));
	onReceiveFileAcceptEventConnection = EventController::getInstance()->connect_onServerEvent(L"file-accept", boost::bind(&FileUploader::onReceiveFileAcceptEvent, this, _1));
	onReceiveFileRejectEventConnection = EventController::getInstance()->connect_onServerEvent(L"file-reject", boost::bind(&FileUploader::onReceiveFileRejectEvent, this, _1));
}

FileUploader::~FileUploader(void)
{
	onReceiveFileOkEventConnection.disconnect();
	onReceiveFileErrorEventConnection.disconnect();
	onReceiveFileAcceptEventConnection.disconnect();
	onReceiveFileRejectEventConnection.disconnect();

	if(fileOpened)
	{
		fclose(pFileStream);
	}
}

void
FileUploader::onReceiveFileOkEvent(const Element& element)
{

}

void
FileUploader::onReceiveFileErrorEvent(const Element& element)
{
	int start = 0;
	int end = 0;
	std::vector<Attribute>::const_iterator it;
	for(it = element.getAttributes().begin(); it != element.getAttributes().end(); it++)
	{
		if((*it).getName() == L"part-start") {
			start = boost::lexical_cast<int>((*it).getValue());
		} else if((*it).getName() == L"part-end") {
			end = boost::lexical_cast<int>((*it).getValue());
		}
	}

	/* If the part-start and part-end attributes are set then send that part again */
	if((start != 0) && (end != 0))
	{
		int size = end - start;
		sendFilePart(start, end);
	} else {
		/* TODO General error */
	}
}

void
FileUploader::onReceiveFileAcceptEvent(const Element& element)
{
	fileAccepted = true;
	SetEvent(hFileAcknowledged);
}

void
FileUploader::onReceiveFileRejectEvent(const Element& element)
{
	SetEvent(hFileAcknowledged);
}

BOOL
FileUploader::getFileSize(std::wstring file, PLARGE_INTEGER fileSize)
{
	/* Open the file and get is size */
	HANDLE hFile;
	hFile = CreateFile(file.c_str(),    // file to open
                   GENERIC_READ,          // open for reading
                   FILE_SHARE_READ,       // share for reading
                   NULL,                  // default security
                   OPEN_EXISTING,         // existing file only
                   FILE_ATTRIBUTE_NORMAL, // normal file
                   NULL);                 // no attr. template
	if(hFile ==  INVALID_HANDLE_VALUE)
	{
		return false;
	} else {
		BOOL gotSize = FALSE;
		gotSize = GetFileSizeEx(hFile, fileSize);
		CloseHandle(hFile);
		return gotSize;
	}
}

bool
FileUploader::sendFile(std::wstring file)
{
	if(!server.isConnected())
	{
		LOG(ERR, "FileUploader: not connected to server so not sending file");
		return false;
	}

	busy = true;

	fileName = file;
	errno_t error;
	

	/* Get the file size */
	if(!getFileSize(file, &fileSize))
	{
		busy = false;
		return false;
	}

	error = _wfopen_s( &pFileStream, file.c_str(), L"rb");
	if(error != 0)
	{
		LOG(ERR, "FileUploader: could not open file: %08x", error);
		busy = false;
		return false;
	} else {
		fileOpened = true;
	}
	
	std::vector<Attribute> atts;
	atts.push_back(Attribute(L"name", file));
	atts.push_back(Attribute(L"size", boost::lexical_cast<std::wstring>(fileSize.QuadPart)));
	atts.push_back(Attribute(L"type", file.substr(file.find_last_of(L".")+1)));
	
	/* Send request to server to accept a file */
	server.sendXML(L"file", atts);

	/* Wait for the server to accept the file or timeout if it fails 
	   to respond or rejects it */
	
	DWORD timeout = WaitForSingleObject(hFileAcknowledged, 5000);

	if((timeout == WAIT_TIMEOUT) || !fileAccepted)
	{
		LOG(ERR, "FileUploader: server did not acknowledge the request to receive a file");
		busy = false;
		if(fileOpened)
		{
			fclose(pFileStream);
			fileOpened = false;
		}
		return false;	
	}	
 
	LOG(INFO, "FileUplodaer: sending file: %ls", fileName.c_str());

	/* Loop sending the file inside <file-part /> xml messages */
	size_t offset = 0;	
	size_t bytesRead;
	do
	{
		bytesRead = sendFilePart((UINT)offset, 8192);
		offset += bytesRead;
	} while(bytesRead > 0);

	//atts.push_back(Attribute(L"name", file));
	//atts.push_back(Attribute(L"size", boost::lexical_cast<std::wstring>(fileSize.QuadPart)));

	server.sendXML(L"file-finished", atts);

	fclose(pFileStream);
	fileOpened = false;
	busy = false;
	return true;
}

size_t
FileUploader::sendFilePart(unsigned int offset, unsigned int size)
{
	if(fileOpened)
	{
		char *pFilePart = (char*)malloc(size);
		Element element;
		element.setName(L"part");

		element.addAttribute(L"name", fileName);
		element.addAttribute(L"part-start", boost::lexical_cast<std::wstring>(offset));
		element.addAttribute(L"encoding", L"base64");

		/* Seek to offset and read size bytes */
		//LOG(INFO, "size: %i\n", size);
		//LOG(INFO, "offset: %i\n", offset);
		int err = fseek(pFileStream, offset, 0);
		//LOG(INFO, "fseel: %i\n", err);
		size_t bytesRead = fread(pFilePart , sizeof(char), size, pFileStream);
		//LOG(INFO, "bytesRead: %i\n", bytesRead);
		//LOG(INFO, "fread error: %i\n", ferror(pFileStream));
		int end = offset + bytesRead;
		element.addAttribute(L"part-end", boost::lexical_cast<std::wstring>(end));

		/* Encode the data just read in base64 and append to XML element */
		size_t encodedLength = 0;
		char* pEncodedFilePart = Base64::encode(pFilePart, bytesRead, &encodedLength);
		element.setData(pEncodedFilePart, encodedLength);

		/* Send the XML element to the server */
		//LOG(INFO, "feof: %i\n", feof(pFileStream));
		if(bytesRead > 0)
		{
			server.sendElement(element);
		}

		/* Cleanup */
		free(pEncodedFilePart);
		free(pFilePart);
		if(feof(pFileStream) > 0)
		{
			return 0;
		} else {
			return bytesRead;
		}
	}
	return 0;
}
