#include "Precompiled.h"

#include "Visitor.h"
#include "Url.h"
#include "Thread.h"
#include "ApplicationPlugin.h"
#include "EventController.h"
#include "VisitEvent.h"
#include <exception>

Visitor::Visitor(void)
{
	LOG(INFO, "here");
	
	visiting = false;

	hQueueNotEmpty = CreateEvent(NULL, FALSE, FALSE, NULL);

	onServerEventConnection = EventController::getInstance()->connect_onServerEvent(L"visit-event", boost::bind(&Visitor::onServerEvent, this, _1));
	//EventController::getInstance()->attach(L"url-group", this);

	loadClientPlugins();

	visitorThread = new Thread(this);
	visitorThread->start("Visitor");
}

Visitor::~Visitor(void)
{
	CloseHandle(hQueueNotEmpty);
	unloadClientPlugins();

	// TODO free items in toVisit queue
}

void
Visitor::run()
{
	try {
		while(true)
		{
			WaitForSingleObject(hQueueNotEmpty, INFINITE);
			VisitEvent* visitEvent = toVisit.front();
			toVisit.pop();
			visiting = true;

			ApplicationPlugin* applicationPlugin = getApplicationPlugin(visitEvent->getProgram());
			

			_ASSERT(applicationPlugin != NULL);
			if(applicationPlugin)
			{
				visitEvent->setAlgorithm(applicationPlugin->getAlgorithm());

				notify(CAPTURE_VISITATION_PRESTART, *visitEvent);

				notify(CAPTURE_VISITATION_START, *visitEvent);

				// Send the group of urls to the appropiate application plugin

				try
				{
					applicationPlugin->visitGroup(visitEvent);
				} 
				catch (...)
				{
					notify(CAPTURE_VISITATION_EXCEPTION, *visitEvent);
				}

				// If there are errors report it else finish visitation
				if(visitEvent->isError())
				{
					notify(visitEvent->getErrorCode(), *visitEvent);
				} else {
					notify(CAPTURE_VISITATION_FINISH, *visitEvent);
				}

				notify(CAPTURE_VISITATION_POSTFINISH, *visitEvent);
			}

			delete visitEvent;
			visiting = false;
		}
	} catch (...) {
		LOG(INFO, "Visitor::run exception\n");	
		throw;
	}
}

void
Visitor::loadClientPlugins()
{

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	wchar_t pluginDirectoryPath[1024];

	GetFullPathName(L"plugins\\Application_*.dll", 1024, pluginDirectoryPath, NULL);
	LOG(INFO, "Capture-Visitor: Plugin directory - %ls\n", pluginDirectoryPath);
	hFind = FindFirstFile(pluginDirectoryPath, &FindFileData);

	if (hFind != INVALID_HANDLE_VALUE) 
	{
		typedef void (*AppPlugin)(void*);
		do
		{
			std::wstring pluginDir = L"plugins\\";
			pluginDir += FindFileData.cFileName;			
			HMODULE hPlugin = LoadLibrary(pluginDir.c_str());

			if(hPlugin != NULL)
			{
				list<ApplicationPlugin*>* apps = new std::list<ApplicationPlugin*>();
				applicationPlugins.insert(PluginPair(hPlugin, apps));
				ApplicationPlugin* applicationPlugin = createApplicationPluginObject(hPlugin);
				if(applicationPlugin == NULL) {
					FreeLibrary(hPlugin);
				} else {
					LOG(INFO, "Loaded plugin: %ls\n", FindFileData.cFileName);
					unsigned int g = applicationPlugin->getPriority();
					wchar_t** supportedApplications = applicationPlugin->getSupportedApplicationNames();
					for(int i = 0; supportedApplications[i] != NULL; i++)
					{
						stdext::hash_map<std::wstring, ApplicationPlugin*>::iterator it;
						it = applicationMap.find(supportedApplications[i]);
						/* Check he application isn't already being handled by a plugin */
						if(it != applicationMap.end())
						{
							/* Check the priority of the existing application plugin */
							unsigned int p = it->second->getPriority();
							if(applicationPlugin->getPriority() > p)
							{
								/* Over ride the exisiting plugin if the priority of the loaded one
								   is greater */
								applicationMap.erase(supportedApplications[i]);
								LOG(INFO, "\toverride: added application: %ls\n", supportedApplications[i]);
								applicationMap.insert(ApplicationPair(supportedApplications[i], applicationPlugin));
							} else {
								LOG(INFO, "\tplugin overridden: not adding application: %ls\n", supportedApplications[i]);
							}
						} else {
							LOG(INFO, "\tinserted: added application: %ls\n", supportedApplications[i]);
							applicationMap.insert(ApplicationPair(supportedApplications[i], applicationPlugin)); 
						}
					}
				}
			} else {
				LOG(INFO, "Unable to load library.\n");
			}
		} while(FindNextFile(hFind, &FindFileData) != 0);
		FindClose(hFind);
	} else {
		LOG(INFO, "Unable to open first plugin.\n");
	}
}

ApplicationPlugin*
Visitor::getApplicationPlugin(const std::wstring& applicationName)
{
	stdext::hash_map<std::wstring, ApplicationPlugin*>::iterator it;
	it = applicationMap.find(applicationName);
	if(it != applicationMap.end())
	{
		return it->second;
	}
	return NULL;
}

ApplicationPlugin*
Visitor::createApplicationPluginObject(HMODULE hPlugin)
{
	typedef void (*PluginExportInterface)(void*);
	PluginExportInterface pluginCreateInstance = NULL;
	ApplicationPlugin* applicationPlugin = NULL;
	/* Get the function address to create a plugin object */
	pluginCreateInstance = (PluginExportInterface)GetProcAddress(hPlugin,"New");
	/* Create a new plugin object in the context of the plugin */
	pluginCreateInstance(&applicationPlugin);
	/* If the object was created then add it to a list so we can track it */
	if(applicationPlugin != NULL)
	{
		stdext::hash_map<HMODULE, std::list<ApplicationPlugin*>*>::iterator it;
		it = applicationPlugins.find(hPlugin);
		if(it != applicationPlugins.end())
		{
			list<ApplicationPlugin*>* apps = it->second;
			apps->push_back(applicationPlugin);
		}
	}
	return applicationPlugin;
}

void
Visitor::unloadClientPlugins()
{
	typedef void (*PluginExportInterface)(void*);
	stdext::hash_map<HMODULE, std::list<ApplicationPlugin*>*>::iterator it;
	for(it = applicationPlugins.begin(); it != applicationPlugins.end(); it++)
	{
		std::list<ApplicationPlugin*>::iterator lit;
		list<ApplicationPlugin*>* apps = it->second;
		PluginExportInterface pluginDeleteInstance = (PluginExportInterface)GetProcAddress(it->first,"Delete");
		for(lit = apps->begin(); lit != apps->end(); lit++)
		{
			pluginDeleteInstance(&(*lit));
		}
		delete apps;
		FreeLibrary(it->first);
	}
}

Url*
Visitor::createUrl(const std::vector<Attribute>& attributes)
{
	std::wstring url;
	std::wstring program;
	int time = 0;
	for each(Attribute attribute in attributes)
	{
		if(attribute.getName() == L"url") {
			url = attribute.getValue();
		} else if(attribute.getName() == L"program") {
			program = attribute.getValue();
		} else if(attribute.getName() == L"time") {
			time = boost::lexical_cast<int>(attribute.getValue());
		}
	}
	return new Url(Url::decode(url), program, time);
}

void
Visitor::onServerEvent(const Element& element)
{
	VisitEvent* visitEvent = new VisitEvent();

	if(element.getName() == L"visit-event") {
		// A url event with multiple urls to visit
		std::wstring identifier = element.getAttributeValue(L"identifier");
		std::wstring program = element.getAttributeValue(L"program");
		int visitTime = boost::lexical_cast<int>(element.getAttributeValue(L"time"));
		
		visitEvent->setIdentifier(identifier);
		visitEvent->setProgram(program);

		for each(Element* e in element.getChildElements())
		{
			if(e->getName() == L"item")
			{
				Url* url = createUrl(e->getAttributes());
				// Force the visit time and program to the url event just in case
				// some supplied them in the item element
				url->setVisitTime(visitTime);
				url->setProgram(program);
				visitEvent->addUrl(url);
			}
		}	
	}

	if(visitEvent->getUrls().size() > 0)
	{
		toVisit.push(visitEvent);
		SetEvent(hQueueNotEmpty);
	} else {
		LOG(INFO, "Visitor-onServerEvent: ERROR no url specified for visit event\n");
		delete visitEvent;
	}
}
