#include "Analyzer.h"
#include "Server.h"
#include "Visitor.h"
#include "ProcessMonitor.h"
#include "ProcessManager.h"
#include "RegistryMonitor.h"
#include "FileMonitor.h"
#include "NetworkPacketDumper.h"
#include "FileUploader.h"
#include "OptionsManager.h"
#include "Logger.h"
#include "Url.h"
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <shellapi.h>

Analyzer::Analyzer(Visitor& v, Server& s) : visitor(v), server(s)
{
	DebugPrintTrace(L"Analyzer::Analyzer(Visitor& v, Server& s) start\n");
	processMonitor = new ProcessMonitor();
	registryMonitor = new RegistryMonitor();
	fileMonitor = new FileMonitor();
	collectModifiedFiles = false;

	if(OptionsManager::getInstance()->getOption(L"capture-network-packets")==L"true") {
		captureNetworkPackets = true;
	} else {
		captureNetworkPackets = false;
	}

	networkPacketDumper = NULL;


	onOptionChangedConnection = OptionsManager::getInstance()->connect_onOptionChanged(boost::bind(&Analyzer::onOptionChanged, this, _1));


	//visitor.onVisitEvent(boost::bind(&Analyzer::onVisitEvent, this, _1, _2, _3, _4));
	visitor.attach(this);
	

	processMonitor->start();
	ProcessManager* processManager = ProcessManager::getInstance();
	processManager->setProcessMonitor(processMonitor);


	registryMonitor->start();
	fileMonitor->start();

	DebugPrintTrace(L"Analyzer::Analyzer(Visitor& v, Server& s) end\n");
}

Analyzer::~Analyzer(void)
{
	DebugPrintTrace(L"Analyzer::~Analyzer(void) start\n");
	/* Do not change the order of these */
	/* The registry monitor must be deleted first ... as when the other monitors
	   kernel drivers are unloaded they cause a lot of registry events which, but
	   they are unloaded before these registry events are finished inside the
	   registry kernel driver. So ... when it comes to unloading the registry
	   kernel driver it will crash ... I guess. I'm not too sure on this as the
	   bug check has nothing to do with Capture instead it has something to do
	   with the PNP manager and deleting the unreferenced device */
	delete registryMonitor;
	delete processMonitor;
	delete fileMonitor;
	if(captureNetworkPackets)
	{
		delete networkPacketDumper;
	}
	DebugPrintTrace(L"Analyzer::~Analyzer(void) end\n");
}

void
Analyzer::onOptionChanged(const std::wstring& option)
{
	DebugPrintTrace(L"Analyzer::onOptionChanged(const std::wstring& option) start\n");
	std::wstring value = OptionsManager::getInstance()->getOption(option); 
	if(captureNetworkPackets || option == L"capture-network-packets-malicious" || 
		option == L"capture-network-packets-benign") {
		if(value == L"true")
		{
			printf("Creating network dumper\n");
			if(networkPacketDumper == NULL) {
				networkPacketDumper = new NetworkPacketDumper();
			}
			captureNetworkPackets = true;
		} else {
			if(captureNetworkPackets == false && OptionsManager::getInstance()->getOption(L"capture-network-packets-malicious") != L"true" &&
				OptionsManager::getInstance()->getOption(L"capture-network-packets-benign") != L"true") {
				captureNetworkPackets = false;
				if(networkPacketDumper != NULL) {
					delete networkPacketDumper;	
				}
				
			}
		}
	} else if(option == L"collect-modified-files") {
		if(value == L"true")
		{
			collectModifiedFiles = true;
		} else {
			collectModifiedFiles = false;
		}
		fileMonitor->setMonitorModifiedFiles(collectModifiedFiles);
	} else if(option == L"send-exclusion-lists") {
		if(value == L"true")
		{
			processMonitor->clearExclusionList();
			registryMonitor->clearExclusionList();
			fileMonitor->clearExclusionList();
		}
	}
	DebugPrintTrace(L"Analyzer::onOptionChanged(const std::wstring& option) end\n");
}

void
Analyzer::start()
{
	DebugPrintTrace(L"Analyzer::start() start\n");
	/* Create the log directory */
	wchar_t* log_directory = new wchar_t[1024];
	GetFullPathName(L"logs", 1024, log_directory, NULL);
	CreateDirectory(log_directory,NULL);
	delete [] log_directory;
	malicious = false;
	if(captureNetworkPackets)
	{
		printf("Start capturing network traffic...\n");
		networkPacketDumper->start();
	}
	onProcessEventConnection = processMonitor->connect_onProcessEvent(boost::bind(&Analyzer::onProcessEvent, this, _1, _2, _3, _4, _5, _6));
	onRegistryEventConnection = registryMonitor->connect_onRegistryEvent(boost::bind(&Analyzer::onRegistryEvent, this, _1, _2, _3, _4, _5));
	onFileEventConnection = fileMonitor->connect_onFileEvent(boost::bind(&Analyzer::onFileEvent, this, _1, _2, _3, _4, _5));
	DebugPrint(L"Analyzer: Registered with callbacks");
	if(collectModifiedFiles)
	{
		fileMonitor->setMonitorModifiedFiles(true);
	}
	DebugPrintTrace(L"Analyzer::start() end\n");

}

void
Analyzer::stop()
{
	DebugPrintTrace(L"Analyzer::stop() start\n");

	onProcessEventConnection.disconnect();
	onRegistryEventConnection.disconnect();
	onFileEventConnection.disconnect();
	DebugPrint(L"Analyzer::stop() stopped monitors\n");

	if(captureNetworkPackets)
	{
		DebugPrint(L"Analyzer::stop() stopping network dumper\n");
		networkPacketDumper->stop();
		DebugPrint(L"Analyzer::stop() stopped network dumper\n");
	}

	if(collectModifiedFiles) 
	{
		fileMonitor->setMonitorModifiedFiles(false);
		fileMonitor->copyCreatedFiles();
	}

	if(OptionsManager::getInstance()->getOption(L"server") != L"") { //only when connected to server
		if(collectModifiedFiles || captureNetworkPackets) //then we zip what we have and send to server
		{
			//if malicious && capture malicious set to false, then delete pcap
			if(malicious && (OptionsManager::getInstance()->getOption(L"capture-network-packets-malicious") == L"false")) {
				networkPacketDumper->deleteAdapterFiles();	
			}
			//if benign && capture benign set to false, then delete pcap
			if(!malicious && (OptionsManager::getInstance()->getOption(L"capture-network-packets-benign") == L"false")) {
				networkPacketDumper->deleteAdapterFiles();
			}

			if(!malicious) {
				DebugPrint(L"Analyzer::stop() deleting deleted_files\n");
				wchar_t* szFullPathDF = new wchar_t[1024];
				GetFullPathName(L"logs\\deleted_files", 1024, szFullPathDF, NULL);
				SHFILEOPSTRUCT deleteDeletedFilesDirectory;
				deleteDeletedFilesDirectory.hwnd = NULL;
				deleteDeletedFilesDirectory.pTo = NULL;
				deleteDeletedFilesDirectory.lpszProgressTitle = NULL;
				deleteDeletedFilesDirectory.wFunc = FO_DELETE;
				deleteDeletedFilesDirectory.pFrom = szFullPathDF;
				deleteDeletedFilesDirectory.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
				SHFileOperation(&deleteDeletedFilesDirectory);
				delete [] szFullPathDF;
				DebugPrint(L"Analyzer::stop() deleted deleted_files\n");

				DebugPrint(L"Analyzer::stop() deleting modified_files\n");
				wchar_t* szFullPathMF = new wchar_t[1024];
				GetFullPathName(L"logs\\modified_files", 1024, szFullPathMF, NULL);
				SHFILEOPSTRUCT deleteModifiedFilesDirectory;
				deleteModifiedFilesDirectory.hwnd = NULL;
				deleteModifiedFilesDirectory.pTo = NULL;
				deleteModifiedFilesDirectory.lpszProgressTitle = NULL;
				deleteModifiedFilesDirectory.wFunc = FO_DELETE;
				deleteModifiedFilesDirectory.pFrom = szFullPathMF;
				deleteModifiedFilesDirectory.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
				SHFileOperation(&deleteModifiedFilesDirectory);
				delete [] szFullPathMF;
				DebugPrint(L"Analyzer::stop() deleted modified_files\n");

			}
			
			SYSTEMTIME st;
			GetLocalTime(&st);

			
			if((malicious && collectModifiedFiles) || (!malicious && (OptionsManager::getInstance()->getOption(L"capture-network-packets-benign") == L"true")) ||
				(malicious && (OptionsManager::getInstance()->getOption(L"capture-network-packets-malicious") == L"true")))
			{
				
				wchar_t* szLogFileName = new wchar_t[1024];

				std::wstring log = L"capture_";
				log += boost::lexical_cast<std::wstring>(st.wDay);
				log += boost::lexical_cast<std::wstring>(st.wMonth);
				log += boost::lexical_cast<std::wstring>(st.wYear);
				log += L"_";
				log += boost::lexical_cast<std::wstring>(st.wHour);
				log += boost::lexical_cast<std::wstring>(st.wMinute);
				log += L".zip";
				GetFullPathName(log.c_str(), 1024, szLogFileName, NULL);

				bool compressed = compressLogDirectory(szLogFileName);
				if(server.isConnected() && compressed)
				{
					printf("Sending log file to server\n");
					FileUploader uploader(server);
					uploader.sendFile(szLogFileName);
					
				}
				if(compressed) 
				{
					DeleteFile(szLogFileName);
				}
				delete [] szLogFileName;
			}
		}

		/* Delete the log directory */
		DebugPrint(L"Analyzer::stop() deleting logs\n");
		wchar_t* szFullPath = new wchar_t[1024];
		GetFullPathName(L"logs", 1024, szFullPath, NULL);
		SHFILEOPSTRUCT deleteLogDirectory;
		deleteLogDirectory.hwnd = NULL;
		deleteLogDirectory.pTo = NULL;
		deleteLogDirectory.lpszProgressTitle = NULL;
		deleteLogDirectory.wFunc = FO_DELETE;
		deleteLogDirectory.pFrom = szFullPath;
		deleteLogDirectory.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
		SHFileOperation(&deleteLogDirectory);
		delete [] szFullPath;
		DebugPrint(L"Analyzer::stop() deleted logs\n");
	}
	DebugPrintTrace(L"Analyzer::stop() end\n");
}

std::wstring
Analyzer::errorCodeToString(DWORD errorCode)
{
	DebugPrintTrace(L"Analyzer::errorCodeToString(DWORD errorCode) start\n");
	wchar_t szTemp[16];
	swprintf_s(szTemp, 16, L"%08x", errorCode);
	std::wstring error = szTemp;
	DebugPrintTrace(L"Analyzer::errorCodeToString(DWORD errorCode) end\n");
	return error;
}



bool
Analyzer::compressLogDirectory(const std::wstring& logFileName)
{
	printf("Analyzer::compressLogDirectory(const std::wstring& logFileName) start\n");
	DebugPrintTrace(L"Analyzer::compressLogDirectory(const std::wstring& logFileName) start\n");
	BOOL created = FALSE;
	STARTUPINFO siStartupInfo;
	PROCESS_INFORMATION piProcessInfo;

	DeleteFile(logFileName.c_str());

	memset(&siStartupInfo, 0, sizeof(siStartupInfo));
	memset(&piProcessInfo, 0, sizeof(piProcessInfo));

	siStartupInfo.cb = sizeof(siStartupInfo);

	wstring processCommand = L"\"";
	processCommand += L"7za.exe";
	processCommand += L"\" a -tzip -y \"";
	processCommand += logFileName.c_str();
	processCommand += L"\" logs";

	created = CreateProcess(NULL,(LPWSTR)processCommand.c_str(), 0, 0, FALSE,
		(CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_CONSOLE), 0, 0, &siStartupInfo,
		&piProcessInfo);

	if( created )
	{
		DWORD err = WaitForSingleObject( piProcessInfo.hProcess, INFINITE );
		printf("Analyzer::compressLogDirectory(const std::wstring& logFileName) end\n");
		DebugPrintTrace(L"Analyzer::compressLogDirectory(const std::wstring& logFileName) end\n");
		return true;
	}
	else
	{
		printf("Analyzer: Cannot open 7za.exe process - 0x%08x\n", GetLastError());
		DebugPrintTrace(L"Analyzer::compressLogDirectory(const std::wstring& logFileName) end\n");
		exit(0);
		return false;
	}
}

void
Analyzer::update(int eventType, const VisitEvent& visitEvent)
{	
	DebugPrintTrace(L"Analyzer::update(int eventType, const VisitEvent& visitEvent) start\n");
	bool send = false;
	std::wstring type;
	std::wstring error_code;
	Element element = visitEvent.toElement();

	switch(eventType)
	{
	case CAPTURE_VISITATION_PRESTART:
		DebugPrint(L"Analyzer::update - Got CAPTURE_VISITATION_PRESTART");
		start();
		break;
	case CAPTURE_VISITATION_START:
		DebugPrint(L"Analyzer::update - Got CAPTURE_VISITATION_START");
		send = true;
		type = L"start";
		break;
	case CAPTURE_VISITATION_FINISH:
		DebugPrint(L"Analyzer::update - Got CAPTURE_VISITATION_FINISH");
		stop();		
		send = true;
		type = L"finish";
		break;
	case CAPTURE_VISITATION_POSTFINISH:
		// sending the finish command used to be here ...
		DebugPrint(L"Analyzer::update - Got CAPTURE_VISITATION_POSTFINISH");
		break;
	default:
		DebugPrint(L"Analyzer::update - Got default");
		stop();	
		send = true;
		type = L"error";
		element.addAttribute(L"error-code", boost::lexical_cast<std::wstring>(eventType));
		break;
	}
	
	if(send)
	{	
		DebugPrint(L"Analyzer::update - sending");
		element.addAttribute(L"type", type);
		//element.addAttribute(L"time", Time::getCurrentTime());
		element.addAttribute(L"malicious", boost::lexical_cast<std::wstring>(malicious));
		server.sendElement(element);
		DebugPrint(L"Analyzer::update - done sending");
	}
	DebugPrintTrace(L"Analyzer::update(int eventType, const VisitEvent& visitEvent) end\n");
}

void
Analyzer::sendSystemEvent(const std::wstring& type, const std::wstring& time, const DWORD processId, 
						  const std::wstring& process, const std::wstring& action, const std::wstring& object1,
					const std::wstring& object2)
{
	DebugPrintTrace(L"Analyzer::sendSystemEvent(...) start\n");
	vector<Attribute> attributes;
	attributes.push_back(Attribute(L"time", time));
	attributes.push_back(Attribute(L"type", type));

	std::wstring sprocessId = boost::lexical_cast<std::wstring>(processId);

	attributes.push_back(Attribute(L"processId", sprocessId)); //parent
	attributes.push_back(Attribute(L"process", process));
	attributes.push_back(Attribute(L"action", action));
	attributes.push_back(Attribute(L"object1", object1));
	attributes.push_back(Attribute(L"object2", object2));
	if(OptionsManager::getInstance()->getOption(L"log-system-events-file") == L"")
	{
		// Output the event to stdout
		printf("%ls: %ls %ls %ls -> %ls %ls\n", type.c_str(), action.c_str(), sprocessId.c_str(), process.c_str(), object2.c_str(), object1.c_str());
	} else {
		// Send the event to the logger
		Logger::getInstance()->writeSystemEventToLog(type, time, sprocessId, process, action, object1, object2);
	}
	server.sendXML(L"system-event", attributes);
	DebugPrintTrace(L"Analyzer::sendSystemEvent(...) end\n");
}

void
Analyzer::onProcessEvent(BOOLEAN created, const std::wstring& time, 
						 DWORD parentProcessId, const std::wstring& parentProcess, 
						 DWORD processId, const std::wstring& process)
{
	DebugPrintTrace(L"Analyzer::onProcessEvent(...) start\n");
	malicious = true;
	std::wstring processEvent = L"process";
	std::wstring processType = L"";
	if(created == TRUE)
	{
		processType = L"created";
	} else {
		processType = L"terminated";
	}

	std::wstring sprocessId = boost::lexical_cast<std::wstring>(processId);

	sendSystemEvent(processEvent, time, parentProcessId,
					parentProcess, processType, sprocessId, 
					process);
	DebugPrintTrace(L"Analyzer::onProcessEvent(...) end\n");
}

void 
Analyzer::onRegistryEvent(const std::wstring& registryEventType, const std::wstring& time, 
						  const DWORD processId, const std::wstring& processPath, const std::wstring& registryEventPath)
{
	DebugPrintTrace(L"Analyzer::onRegistryEvent(...) start\n");
	malicious = true;
	std::wstring dummy = L"-1";
	std::wstring registryEvent = L"registry";
	sendSystemEvent(registryEvent, time, processId,
		processPath, registryEventType, registryEventPath, dummy);
	DebugPrintTrace(L"Analyzer::onRegistryEvent(...) end\n");
}

void
Analyzer::onFileEvent(const std::wstring& fileEventType, const std::wstring& time, const DWORD processId, 
						 const std::wstring& processPath, const std::wstring& fileEventPath)
{
	DebugPrintTrace(L"Analyzer::onFileEvent(...) start\n");
	malicious = true;
	std::wstring fileEvent = L"file";
	std::wstring dummy = L"-1";
	sendSystemEvent(fileEvent, time, processId,
					processPath, fileEventType, fileEventPath, dummy);
	DebugPrintTrace(L"Analyzer::onFileEvent(...) end\n");
}
