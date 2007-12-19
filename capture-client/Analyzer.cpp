#include "Analyzer.h"
#include "Server.h"
#include "Visitor.h"
#include "ProcessMonitor.h"
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
	processMonitor = new ProcessMonitor();
	registryMonitor = new RegistryMonitor();
	fileMonitor = new FileMonitor();
	collectModifiedFiles = false;
	captureNetworkPackets = false;
	networkPacketDumper = NULL;


	onOptionChangedConnection = OptionsManager::getInstance()->connect_onOptionChanged(boost::bind(&Analyzer::onOptionChanged, this, _1));


	//visitor.onVisitEvent(boost::bind(&Analyzer::onVisitEvent, this, _1, _2, _3, _4));
	visitor.attach(this);
	

	processMonitor->start();
	registryMonitor->start();
	fileMonitor->start();
}

Analyzer::~Analyzer(void)
{
	DebugPrint(L"Analyzer: Deconstructor\n");
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
}

void
Analyzer::onOptionChanged(const std::wstring& option)
{
	std::wstring value = OptionsManager::getInstance()->getOption(option); 
	if(option == L"capture-network-packets-malicious" || 
		option == L"capture-network-packets-benign") {
		if(value == L"true")
		{
			if(captureNetworkPackets == false)
			{
				printf("Creating network dumper\n");
				if(networkPacketDumper == NULL)
					networkPacketDumper = new NetworkPacketDumper();
				captureNetworkPackets = true;
			}
		} else {
			if(captureNetworkPackets == true)
			{
				if(OptionsManager::getInstance()->getOption(L"capture-network-packets-malicious") != L"true" &&
					OptionsManager::getInstance()->getOption(L"capture-network-packets-benign") != L"true")
				{
					captureNetworkPackets = false;
					if(networkPacketDumper != NULL)
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
}

void
Analyzer::start()
{
	/* Create the log directory */
	wchar_t* log_directory = new wchar_t[1024];
	GetFullPathName(L"logs", 1024, log_directory, NULL);
	CreateDirectory(log_directory,NULL);
	delete [] log_directory;
	malicious = false;
	DebugPrint(L"Analyzer: Start");
	if(captureNetworkPackets)
	{
		networkPacketDumper->start();
	}
	onProcessEventConnection = processMonitor->connect_onProcessEvent(boost::bind(&Analyzer::onProcessEvent, this, _1, _2, _3, _4, _5, _6));
	onRegistryEventConnection = registryMonitor->connect_onRegistryEvent(boost::bind(&Analyzer::onRegistryEvent, this, _1, _2, _3, _4));
	onFileEventConnection = fileMonitor->connect_onFileEvent(boost::bind(&Analyzer::onFileEvent, this, _1, _2, _3, _4));
	DebugPrint(L"Analyzer: Registered with callbacks");
	if(collectModifiedFiles)
	{
		fileMonitor->setMonitorModifiedFiles(true);
	}
}

void
Analyzer::stop()
{
	DebugPrint(L"Analyzer: Stop");
	onProcessEventConnection.disconnect();
	onRegistryEventConnection.disconnect();
	onFileEventConnection.disconnect();

	if(captureNetworkPackets)
	{
		networkPacketDumper->stop();
	}

	if(collectModifiedFiles || captureNetworkPackets)
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		if(collectModifiedFiles) 
		{
			fileMonitor->setMonitorModifiedFiles(false);
			fileMonitor->copyCreatedFiles();
		}

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

		if((!malicious && (OptionsManager::getInstance()->getOption(L"capture-network-packets-benign") == L"true")) ||
			(malicious && (OptionsManager::getInstance()->getOption(L"capture-network-packets-malicious") == L"true")))
		{
			if(server.isConnected() && compressed)
			{				
				FileUploader uploader(server);
				uploader.sendFile(szLogFileName);
				DeleteFile(szLogFileName);
			}
		}

		delete [] szLogFileName;
	}
	/* Delete the log directory */
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
		CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo,
		&piProcessInfo);

	DWORD err = WaitForSingleObject( piProcessInfo.hProcess, INFINITE );

	//DebugPrint(L"Analyzer: compressLogDirectory - WaitForSingleObject = 0x%08x, CreateProcess=%i", err, created);
	if(!created)
	{
		printf("Analyzer: Cannot open 7za.exe process - 0x%08x\n", GetLastError());
		printf("Log directory not compressed ... \n");
		return false;
	} else {
		return true;
	}
}

void
Analyzer::update(int eventType, const VisitEvent& visitEvent)
{	
	bool send = false;
	std::wstring type;

	switch(eventType)
	{
	case CAPTURE_VISITATION_PRESTART:
		start();
		break;
	case CAPTURE_VISITATION_START:
		send = true;
		type = L"start";
		break;
	case CAPTURE_VISITATION_FINISH:
		stop();
		break;
	case CAPTURE_VISITATION_POSTFINISH:
		send = true;
		type = L"finish";
		break;
	default:
		send = true;
		type = L"error";
		break;
	}

	if(send)
	{
		Element element = visitEvent.toElement();
		element.addAttribute(L"type", type);
		//element.addAttribute(L"time", Time::getCurrentTime());
		element.addAttribute(L"malicious", boost::lexical_cast<std::wstring>(malicious));
		server.sendElement(element);
	}
}

void
Analyzer::sendSystemEvent(const std::wstring& type, const std::wstring& time, 
					const std::wstring& process, const std::wstring& action, 
					const std::wstring& object)
{
	vector<Attribute> attributes;
	attributes.push_back(Attribute(L"time", time));
	attributes.push_back(Attribute(L"type", type));
	attributes.push_back(Attribute(L"process", process));
	attributes.push_back(Attribute(L"action", action));
	attributes.push_back(Attribute(L"object", object));
	if(OptionsManager::getInstance()->getOption(L"log-system-events-file") == L"")
	{
		// Output the event to stdout
		printf("%ls: %ls %ls -> %ls\n", type.c_str(), action.c_str(), process.c_str(), object.c_str());
	} else {
		// Send the event to the logger
		Logger::getInstance()->writeSystemEventToLog(type, time, process, action, object);
	}
	server.sendXML(L"system-event", attributes);
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
	sendSystemEvent(processEvent, time, 
					parentProcess, processType, 
					process);
}

void 
Analyzer::onRegistryEvent(const std::wstring& registryEventType, const std::wstring& time, 
						  const std::wstring& processPath, const std::wstring& registryEventPath)
{
	malicious = true;
	std::wstring registryEvent = L"registry";
	sendSystemEvent(registryEvent, time, 
					processPath, registryEventType, 
					registryEventPath);
}

void
Analyzer::onFileEvent(const std::wstring& fileEventType, const std::wstring& time, 
						 const std::wstring& processPath, const std::wstring& fileEventPath)
{
	malicious = true;
	std::wstring fileEvent = L"file";
	sendSystemEvent(fileEvent, time, 
					processPath, fileEventType, 
					fileEventPath);
}
