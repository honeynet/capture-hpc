#include "FileMonitor.h"
#include "ProcessManager.h"
#include "EventController.h"
#include "OptionsManager.h"
#include "Logger.h"
#include <shlobj.h>
#include <boost/bind.hpp>

FileMonitor::FileMonitor(void)
{
	HRESULT hResult;
	monitorRunning = false;
	driverInstalled = false;
	int turn = 0;
	communicationPort = INVALID_HANDLE_VALUE;
	monitorModifiedFiles = false;

	hMonitorStoppedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	FilterUnload(L"CaptureFileMonitor");
	hResult = FilterLoad(L"CaptureFileMonitor");
	// Keep trying to load the filter - On some VM's this can take a few seconds
#ifdef NDEBUG
	while((turn < 5) && IS_ERROR( hResult ))
	{
		turn++;
		printf("FileMonitor: WARNING - Filter driver not loaded (error: %x) waiting 3 seconds to try again ... (try %i of 5)\n", hResult, turn);
		Sleep(3000);
		hResult = FilterLoad(L"CaptureFileMonitor");
	}
#endif
	if (!IS_ERROR( hResult )) {
		
		hResult = FilterConnectCommunicationPort( CAPTURE_FILEMON_PORT_NAME,
												0,
												NULL,
												0,
												NULL,
												&communicationPort );
		if (IS_ERROR( hResult )) {

			printf( "FileMonitor: ERROR - Could not connect to filter: 0x%08x\n", hResult );
		} else {
			printf("Loaded filter driver: CaptureFileMonitor\n");
			driverInstalled = true;
			//this.setMonitorModifiedFiles(false);
		}
		
		wchar_t exListDriverPath[1024];
		GetFullPathName(L"FileMonitor.exl", 1024, exListDriverPath, NULL);
		Monitor::loadExclusionList(exListDriverPath);

		/* Create a log directory exclusion which allows all writes in
		   Captures log directory */
		wchar_t logDirectory[1024];
		GetFullPathName(L"logs", 1024, logDirectory, NULL);
		std::wstring captureLogDirectory = logDirectory;
		Monitor::prepareStringForExclusion(captureLogDirectory);
		captureLogDirectory += L"\\\\.*";

		std::wstring captureExecutablePath = ProcessManager::getInstance()->getProcessPath(GetCurrentProcessId());
		Monitor::prepareStringForExclusion(captureExecutablePath);
		
		/* Exclude file events in the log directory */
		/* NOTE we exclude all processes because files are copied/delete/openend
		   etc in the context of the calling process not Capture */
		Monitor::addExclusion(L"+", L"write", L".*", captureLogDirectory, true);
		Monitor::addExclusion(L"+", L"create", L".*", captureLogDirectory, true);
		Monitor::addExclusion(L"+", L"delete", L".*", captureLogDirectory, true);
		Monitor::addExclusion(L"+", L"open", L".*", captureLogDirectory, true);
		//Monitor::addExclusion(L"+", L"open", L".*", logDirectory, true);
		Monitor::addExclusion(L"+", L"read", L".*", captureLogDirectory, true);

		/* Exclude Captures temp directory */
		wchar_t tempDirectory[1024];
		GetFullPathName(L"temp", 1024, tempDirectory, NULL);
		std::wstring captureTempDirectory = tempDirectory;
		Monitor::prepareStringForExclusion(captureTempDirectory);
		captureTempDirectory += L"\\\\.*";

		//std::wstring captureExecutablePath = ProcessManager::getInstance()->getProcessPath(GetCurrentProcessId());
		//Monitor::prepareStringForExclusion(&captureExecutablePath);
		
		/* Exclude file events in the log directory */
		/* NOTE we exclude all processes because files are copied/delete/openend
		   etc in the context of the calling process not Capture */
		Monitor::addExclusion(L"+", L"write", L".*", captureTempDirectory, true);
		Monitor::addExclusion(L"+", L"create", L".*", captureTempDirectory, true);
		Monitor::addExclusion(L"+", L"delete", L".*", captureTempDirectory, true);
		Monitor::addExclusion(L"+", L"open", L".*", captureTempDirectory, true);
		Monitor::addExclusion(L"+", L"read", L".*", captureTempDirectory, true);
		
		if(OptionsManager::getInstance()->getOption(L"log-system-events-file") != L"")
		{
			std::wstring loggerFile = Logger::getInstance()->getLogFullPath();
			Monitor::prepareStringForExclusion(loggerFile);
			Monitor::addExclusion(L"+", L"create", captureExecutablePath, loggerFile, true);
			Monitor::addExclusion(L"+", L"write", captureExecutablePath, loggerFile, true);
		}
		onFileExclusionReceivedConnection = EventController::getInstance()->connect_onServerEvent(L"file-exclusion", boost::bind(&FileMonitor::onFileExclusionReceived, this, _1));

		initialiseDosNameMap();
	}
}

void
FileMonitor::setMonitorModifiedFiles(bool monitor)
{
	FILEMONITOR_MESSAGE command;
	FILEMONITOR_SETUP setupFileMonitor;
	HRESULT hResult;
	DWORD bytesReturned;

	monitorModifiedFiles = monitor;

	if(monitor == true)
	{
		wchar_t* modified_files_directory = new wchar_t[1024];
		GetFullPathName(L"logs\\modified_files", 1024, modified_files_directory, NULL);
		CreateDirectory(modified_files_directory,NULL);
		setupFileMonitor.bCollectDeletedFiles = TRUE;
		delete [] modified_files_directory;
	} else {
		setupFileMonitor.bCollectDeletedFiles = FALSE;
	}
	GetFullPathName(L"logs\\deleted_files", 1024, setupFileMonitor.wszLogDirectory, NULL);
	CreateDirectory(setupFileMonitor.wszLogDirectory,NULL);
	setupFileMonitor.nLogDirectorySize = (UINT)wcslen(setupFileMonitor.wszLogDirectory)*sizeof(wchar_t);
	
	DebugPrint(L"Deleted file directory: %i -> %ls\n", setupFileMonitor.nLogDirectorySize, setupFileMonitor.wszLogDirectory);

	command.Command = SetupMonitor;	

	hResult = FilterSendMessage( communicationPort,
							 &command,
							 sizeof( FILEMONITOR_COMMAND ),
							 &setupFileMonitor,
							 sizeof(FILEMONITOR_SETUP),
							 &bytesReturned );
}

FileMonitor::~FileMonitor(void)
{
	stop();
	if(isDriverInstalled())
	{
		driverInstalled = false;
		CloseHandle(communicationPort);
		FilterUnload(L"CaptureFileMonitor");
	}
	CloseHandle(hMonitorStoppedEvent);
}

void 
FileMonitor::initialiseDosNameMap()
{
	UCHAR buffer[1024];
	PFILTER_VOLUME_BASIC_INFORMATION volumeBuffer = (PFILTER_VOLUME_BASIC_INFORMATION)buffer;
	HANDLE volumeIterator = INVALID_HANDLE_VALUE;
	ULONG volumeBytesReturned;
	WCHAR driveLetter[15];

	HRESULT hResult = FilterVolumeFindFirst( FilterVolumeBasicInformation,
										 volumeBuffer,
										 sizeof(buffer)-sizeof(WCHAR),   //save space to null terminate name
										 &volumeBytesReturned,
										 &volumeIterator );
	if (volumeIterator != INVALID_HANDLE_VALUE) {
		do {
			assert((FIELD_OFFSET(FILTER_VOLUME_BASIC_INFORMATION,FilterVolumeName) + volumeBuffer->FilterVolumeNameLength) <= (sizeof(buffer)-sizeof(WCHAR)));
			__analysis_assume((FIELD_OFFSET(FILTER_VOLUME_BASIC_INFORMATION,FilterVolumeName) + volumeBuffer->FilterVolumeNameLength) <= (sizeof(buffer)-sizeof(WCHAR)));
			volumeBuffer->FilterVolumeName[volumeBuffer->FilterVolumeNameLength/sizeof( WCHAR )] = UNICODE_NULL;
				
			if(SUCCEEDED( FilterGetDosName(volumeBuffer->FilterVolumeName,
				driveLetter,
				sizeof(driveLetter)/sizeof(WCHAR) )
				))
			{
				std::wstring dLetter = driveLetter;
				dosNameMap.insert(std::pair<std::wstring, std::wstring>(volumeBuffer->FilterVolumeName, dLetter));
			}
		} while (SUCCEEDED( hResult = FilterVolumeFindNext( volumeIterator,
															FilterVolumeBasicInformation,
															volumeBuffer,
															sizeof(buffer)-sizeof(WCHAR),    //save space to null terminate name
															&volumeBytesReturned ) ));
	}

	if (INVALID_HANDLE_VALUE != volumeIterator) {
		FilterVolumeFindClose( volumeIterator );
	}
}

boost::signals::connection 
FileMonitor::connect_onFileEvent(const signal_fileEvent::slot_type& s)
{ 
	return signal_onFileEvent.connect(s); 
}

bool
FileMonitor::isDirectory(std::wstring filePath)
{
	DWORD code = GetFileAttributes(filePath.c_str());
	if (code==INVALID_FILE_ATTRIBUTES)
		return false;
	if ((code&FILE_ATTRIBUTE_DIRECTORY)==FILE_ATTRIBUTE_DIRECTORY) 
		return true;
	return false;
}

void
FileMonitor::onFileExclusionReceived(const Element& element)
{
	std::wstring excluded = L"";
	std::wstring fileEventType = L"";
	std::wstring processPath = L"";
	std::wstring filePath = L"";

	std::vector<Attribute>::const_iterator it;
	for(it = element.getAttributes().begin(); it != element.getAttributes().end(); it++)
	{
		if((*it).getName() == L"action") {
			fileEventType = (*it).getValue();
		} else if((*it).getName() == L"object") {
			filePath = (*it).getValue();
		} else if((*it).getName() == L"subject") {
			processPath = (*it).getValue();
		} else if((*it).getName() == L"excluded") {
			excluded = (*it).getValue();
		}
	}
	Monitor::addExclusion(excluded, fileEventType, processPath, filePath);
}

void
FileMonitor::start()
{
	if(!isMonitorRunning() && isDriverInstalled())
	{
		fileEvents = (BYTE*)malloc(FILE_EVENTS_BUFFER_SIZE);
		fileMonitorThread = new Thread(this);
		fileMonitorThread->start("FileMonitor");
	}
}

void
FileMonitor::stop()
{	
	if(isMonitorRunning() && isDriverInstalled())
	{
		monitorRunning = false;
		WaitForSingleObject(hMonitorStoppedEvent, 1000);
		fileMonitorThread->stop();
		delete fileMonitorThread;
		free(fileEvents);
	}	
}

bool
FileMonitor::getFileEventName(PFILE_EVENT pFileEvent, std::wstring *fileEventName)
{
	bool found = true;
	*fileEventName = L"UNKNOWN";
	if(pFileEvent->majorFileEventType == IRP_MJ_CREATE) {
		/* Defined at http://msdn2.microsoft.com/en-us/library/ms804358.aspx */
		//if(!FlagOn(pFileEvent->flags, FO_STREAM_FILE) && pFileEvent->flags > 0)
		//{
			if( pFileEvent->information == FILE_CREATED || 
				pFileEvent->information == FILE_OVERWRITTEN ||
				pFileEvent->information == FILE_SUPERSEDED ) {
				*fileEventName = L"Create";
			} else if(pFileEvent->information == FILE_OPENED) {
				*fileEventName = L"Open";
			} else {
				found = false;
			}
		//} else {
		//	found = false;
		//}
			/* Ignore stream accesses */
			std::wstring eventPath = pFileEvent->filePath;
			if(eventPath.find(L":", 0) != std::wstring::npos)
			{
				found = false;
			}
	} else if(pFileEvent->majorFileEventType == IRP_MJ_READ) {
		*fileEventName = L"Read";
	} else if(pFileEvent->majorFileEventType == IRP_MJ_WRITE) {
		*fileEventName = L"Write";
	} else if(pFileEvent->majorFileEventType == IRP_MJ_DELETE) {
		*fileEventName = L"Delete";
	//} else if(pFileEvent->majorFileEventType == IRP_MJ_CLOSE) {
	//	*fileEventName = L"Close";
	} else {
		found = false;
	}
	return found;
}

std::wstring 
FileMonitor::convertFileObjectNameToDosName(std::wstring fileObjectName)
{
	stdext::hash_map<std::wstring, std::wstring>::iterator it;
	for(it = dosNameMap.begin(); it != dosNameMap.end(); it++)
	{
		size_t position = fileObjectName.rfind(it->first,0);
		if(position != std::wstring::npos)
		{
			fileObjectName.replace(position, it->first.length(), it->second, 0, it->second.length());
			break;
		}
	}
	//transform(fileObjectName.begin(), fileObjectName.end(), fileObjectName.begin(), tolower);
	return fileObjectName;
}

void
FileMonitor::createFilePathAndCopy(std::wstring* logPath, std::wstring* filePath)
{
	printf("Copying file: %ls\n", filePath->c_str());
	std::wstring drive = filePath->substr(0,filePath->find_first_of(L":"));
	std::wstring fileName = filePath->substr(filePath->find_last_of(L"\\")+1);
	std::wstring intermediateDirectory = *logPath;
	intermediateDirectory += drive;
	intermediateDirectory += L"\\";
	intermediateDirectory += filePath->substr(filePath->find_first_of(L"\\")+1,filePath->find_last_of(L"\\")-filePath->find_first_of(L"\\")-1);
	SHCreateDirectoryEx(NULL,
							intermediateDirectory.c_str(),
							NULL
							);
	std::wstring filePathAndName = intermediateDirectory;
	filePathAndName += L"\\";
	filePathAndName += fileName;
	if(!CopyFile(
		filePath->c_str(),
		filePathAndName.c_str(),
		FALSE
		))
	{
		printf("\t... failed: 0x%08x\n", GetLastError());
	} else {
		printf("\t... done\n");
	}
}

void
FileMonitor::copyCreatedFiles()
{
	stdext::hash_set<std::wstring>::iterator it;
	wchar_t currentDirectory[1024];
	GetFullPathName(L"logs\\modified_files\\", 1024, currentDirectory, NULL);
	std::wstring logPath = currentDirectory;
	DebugPrint(L"Modified file directory: %ls\n", logPath.c_str());
	for(it = modifiedFiles.begin(); it != modifiedFiles.end(); it++)
	{
		std::wstring filePath = *it;
		createFilePathAndCopy(&logPath, &filePath );
	}
	modifiedFiles.clear();
}

void
FileMonitor::run()
{
	try {
		HRESULT hResult;
		DWORD bytesReturned = 0;
		monitorRunning = true;
		while(isMonitorRunning())
		{
			FILEMONITOR_MESSAGE command;
			command.Command = GetFileEvents;

			ZeroMemory(fileEvents, FILE_EVENTS_BUFFER_SIZE);

			hResult = FilterSendMessage( communicationPort,
										 &command,
										 sizeof( FILEMONITOR_COMMAND ),
										 fileEvents,
										 FILE_EVENTS_BUFFER_SIZE,
										 &bytesReturned );
			if(bytesReturned >= sizeof(FILE_EVENT))
			{
				UINT offset = 0;
				do {
					PFILE_EVENT e = (PFILE_EVENT)(fileEvents + offset);

					std::wstring fileEventName;
					std::wstring fileEventPath;
					std::wstring processModuleName;
					std::wstring processPath;

					if(getFileEventName(e, &fileEventName))
					{
						processPath = ProcessManager::getInstance()->getProcessPath(e->processId);
						processModuleName = ProcessManager::getInstance()->getProcessModuleName(e->processId);
						
						fileEventPath = e->filePath;
						fileEventPath = convertFileObjectNameToDosName(fileEventPath);

						if((fileEventPath != L"UNKNOWN"))
						{
							if(!Monitor::isEventAllowed(fileEventName, processPath, fileEventPath))
							{
								if(monitorModifiedFiles)
								{
			
									if(!isDirectory(fileEventPath))
									{
										if(e->majorFileEventType == IRP_MJ_CREATE || 
											e->majorFileEventType == IRP_MJ_WRITE )
										{	
											modifiedFiles.insert(fileEventPath);
										} else if(e->majorFileEventType == IRP_MJ_DELETE) 
										{
											modifiedFiles.erase(fileEventPath);
										}	
									}
								}

								signal_onFileEvent(fileEventName, Time::timefieldToString(e->time), processPath, fileEventPath);
							}
						}
					}
					
					offset += sizeof(FILE_EVENT) + e->filePathLength;
				} while(offset < bytesReturned);	
			}
			
			if(bytesReturned == FILE_EVENTS_BUFFER_SIZE)
			{
				Sleep(FILE_EVENT_BUFFER_FULL_WAIT_TIME);
			} else {
				Sleep(FILE_EVENT_WAIT_TIME);
			}
		}
		SetEvent(hMonitorStoppedEvent);
	} catch (...) {
		printf("FileMonitor::run exception\n");	
		throw;
	}
}