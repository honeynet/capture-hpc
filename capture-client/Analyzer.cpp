#include "Precompiled.h"

#include "Analyzer.h"
#include "Server.h"
#include "Visitor.h"
#include "ProcessMonitor.h"
#include "ProcessManager.h"
#include "RegistryMonitor.h"
#include "FileMonitor.h"
#include "NetworkMonitor.h"
#include "NetworkPacketDumper.h"
#include "FileUploader.h"
#include "OptionsManager.h"
#include "Url.h"
#include <shellapi.h>

Analyzer::Analyzer(Visitor& v, Server& s) : visitor(v), server(s)
{
	LOG(INFO, "Analyzer: initializing");
	processMonitor = new ProcessMonitor();
	registryMonitor = new RegistryMonitor();
	networkMonitor = new NetworkMonitor();
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
	networkMonitor->start();

	LOG(INFO, "Analyzer: initialized");
}

Analyzer::~Analyzer(void)
{
	LOG(INFO, "Analyzer: destroying");
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
	delete networkMonitor;
	if(captureNetworkPackets)
	{
		delete networkPacketDumper;
	}
	LOG(INFO, "Analyzer: destroyed");
}

void
Analyzer::onOptionChanged(const std::wstring& option)
{
	LOG(INFO, "Analyzer: option changed - %ls", option.c_str());

	std::wstring value = OptionsManager::getInstance()->getOption(option); 
	if(option == L"capture-network-packets-malicious" || 
		option == L"capture-network-packets-benign" || option == L"capture-network-packets") {
		if(value == L"true")
		{
			if(networkPacketDumper == NULL) {
				LOG(INFO, "Creating network dumper");
				networkPacketDumper = new NetworkPacketDumper();
			}
			captureNetworkPackets = true;
		} else {
			if(captureNetworkPackets == false && OptionsManager::getInstance()->getOption(L"capture-network-packets-malicious") != L"true" &&
				OptionsManager::getInstance()->getOption(L"capture-network-packets-benign") != L"true") {
				captureNetworkPackets = false;
				
				if(networkPacketDumper != NULL) {
					LOG(INFO, "Analyzer: deleting network dumper");
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
			networkMonitor->clearExclusionList();
		}
	}
}

void
Analyzer::start()
{
	LOG(INFO, "Analyzer: starting");

	/* Create the log directory */
	wchar_t* log_directory = new wchar_t[1024];
	GetFullPathName(L"logs", 1024, log_directory, NULL);
	CreateDirectory(log_directory,NULL);
	delete [] log_directory;
	malicious = false;
	if(captureNetworkPackets)
	{
		LOG(INFO, "Analyzer: starting network packet dumper");
		networkPacketDumper->start();
	}
	onProcessEventConnection = processMonitor->connect_onProcessEvent(boost::bind(&Analyzer::onProcessEvent, this, _1, _2, _3, _4, _5, _6));
	onRegistryEventConnection = registryMonitor->connect_onRegistryEvent(boost::bind(&Analyzer::onRegistryEvent, this, _1, _2, _3, _4, _5));
	onFileEventConnection = fileMonitor->connect_onFileEvent(boost::bind(&Analyzer::onFileEvent, this, _1, _2, _3, _4, _5));
	/*TODO check this is right*/
	onNetworkEventConnection = networkMonitor->connect_onConnectionEvent(boost::bind(&Analyzer::onConnectionEvent, this, _1, _2, _3, _4, _5));
	LOG(INFO, "Analyzer: registered event callbacks");
	if(collectModifiedFiles)
	{
		LOG(INFO, "Analyzer: capturing modified files");
		fileMonitor->setMonitorModifiedFiles(true);
	}

	LOG(INFO, "Analyzer: started");
}

void
Analyzer::stop()
{
	LOG(INFO, "Analyzer: stopping");

	onProcessEventConnection.disconnect();
	onRegistryEventConnection.disconnect();
	onFileEventConnection.disconnect();
	onNetworkEventConnection.disconnect();

	if(captureNetworkPackets)
	{
		LOG(INFO, "Analyzer: stopping network dumper");
		networkPacketDumper->stop();
	}

	if(collectModifiedFiles) 
	{
		LOG(INFO, "Analyzer: copying modified files");
		fileMonitor->setMonitorModifiedFiles(false);
		fileMonitor->copyCreatedFiles();
	}

	if(OptionsManager::getInstance()->getOption(L"server") != L"") { //only when connected to server
		if(collectModifiedFiles || captureNetworkPackets) //then we zip what we have and send to server
		{
			//if malicious && capture malicious set to false, then delete pcap
			if(malicious && (OptionsManager::getInstance()->getOption(L"capture-network-packets-malicious") == L"false")) {
				if(networkPacketDumper)
					networkPacketDumper->deleteAdapterFiles();	
			}
			//if benign && capture benign set to false, then delete pcap
			if(!malicious && (OptionsManager::getInstance()->getOption(L"capture-network-packets-benign") == L"false")) {
				if(networkPacketDumper)
					networkPacketDumper->deleteAdapterFiles();
			}

			if(!malicious) {
				LOG(INFO, "Analyzer: deleting modified files");
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
					LOG(INFO, "Analyzer: sending log file to server");
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
		LOG(INFO, "Analyzer: deleting logs");
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
	}
	LOG(INFO, "Analyzer: stopped");
}

std::wstring
Analyzer::errorCodeToString(DWORD errorCode)
{
	wchar_t szTemp[16];
	swprintf_s(szTemp, 16, L"%08x", errorCode);
	std::wstring error = szTemp;
	return error;
}



bool
Analyzer::compressLogDirectory(const std::wstring& logFileName)
{
	LOG(INFO, "Analyzer: compressing log directory");

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
		LOG(INFO, "Analyzer: waiting for log file compressing to complete");
		DWORD err = WaitForSingleObject( piProcessInfo.hProcess, INFINITE );
		return true;
	}
	else
	{
		LOG(ERR, "Analyzer: cannot open 7za.exe process - 0x%08x", GetLastError());
		exit(0);
		return false;
	}
}

void
Analyzer::update(int eventType, const VisitEvent& visitEvent)
{	
	bool send = false;
	std::wstring type;
	std::wstring error_code;
	Element element = visitEvent.toElement();

	switch(eventType)
	{
	case CAPTURE_VISITATION_PRESTART:
		LOG(INFO, "Analyzer: visitation pre-start");
		start();
		break;
	case CAPTURE_VISITATION_START:
		LOG(INFO, "Analyzer: visitation start");
		send = true;
		type = L"start";
		break;
	case CAPTURE_VISITATION_FINISH:
		LOG(INFO, "Analyzer: visitation finish");
		stop();		
		send = true;
		type = L"finish";
		break;
	case CAPTURE_VISITATION_POSTFINISH:
		// sending the finish command used to be here ...
		LOG(INFO, "Analyzer: visitation post-finish");
		break;
	default:
		LOG(INFO, "Analyzer: visitation default");
		stop();	
		send = true;
		type = L"error";
		element.addAttribute(L"error-code", boost::lexical_cast<std::wstring>(eventType));
		break;
	}
	
	if(send)
	{	
		LOG(INFO, "Analyzer: visitation sending malicious event");
		element.addAttribute(L"type", type);
		//element.addAttribute(L"time", Time::getCurrentTime());
		element.addAttribute(L"malicious", boost::lexical_cast<std::wstring>(malicious));
		server.sendElement(element);
	}
}

void
Analyzer::sendSystemEvent(const std::wstring& type, const std::wstring& time, const DWORD processId, 
						  const std::wstring& process, const std::wstring& action, const std::wstring& object1,
					const std::wstring& object2)
{
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
		LOG(EVENT, "%ls: %ls %ls %ls -> %ls %ls", type.c_str(), action.c_str(), sprocessId.c_str(), process.c_str(), object2.c_str(), object1.c_str());
	} else {
		// Send the event to the logger
		Logger::getInstance()->writeSystemEventToLog(type, time, sprocessId, process, action, object1, object2);
	}
	server.sendXML(L"system-event", attributes);

	LOG(INFO, "Analyzer: sent event to server - %ls", type.c_str());
}

void
Analyzer::onProcessEvent(BOOLEAN created, const std::wstring& time, 
						 DWORD parentProcessId, const std::wstring& parentProcess, 
						 DWORD processId, const std::wstring& process)
{
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
}

void 
Analyzer::onRegistryEvent(const std::wstring& registryEventType, const std::wstring& time, 
						  const DWORD processId, const std::wstring& processPath, const std::wstring& registryEventPath)
{
	malicious = true;
	std::wstring dummy = L"-1";
	std::wstring registryEvent = L"registry";
	sendSystemEvent(registryEvent, time, processId,
		processPath, registryEventType, registryEventPath, dummy);
}

void
Analyzer::onFileEvent(const std::wstring& fileEventType, const std::wstring& time, const DWORD processId, 
						 const std::wstring& processPath, const std::wstring& fileEventPath)
{
	malicious = true;
	std::wstring fileEvent = L"file";
	std::wstring dummy = L"-1";
	sendSystemEvent(fileEvent, time, processId,
					processPath, fileEventType, fileEventPath, dummy);
}

void
Analyzer::onConnectionEvent(const std::wstring& networkEventType, const std::wstring& time, const DWORD processId, 
						 const std::wstring& processPath, const std::wstring& networkEventPath)
{
	malicious = true;
	std::wstring networkEvent = L"connection";
	std::wstring dummy = L"-1";
	sendSystemEvent(networkEvent, time, processId,
					processPath, networkEventType, networkEventPath, dummy);
}
