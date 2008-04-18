#include "PluginTest.h"

PluginTest::PluginTest(void)
{
}

PluginTest::~PluginTest(void)
{
}

void PluginTest::loadTest() {
	loadIEPlugin();

	for(int i=0;i<100;i++) {
		VisitEvent* visitEvent = new VisitEvent();
		visitEvent->setIdentifier(L"1");
		visitEvent->setProgram(L"iexplore");
		
		Url* url = new Url(L"http://www.google.com",L"iexplore",10);
		visitEvent->addUrl(url);
		
		Url* url2 = new Url(L"http://www.google.de",L"iexplore",10);
		visitEvent->addUrl(url2);
		
		ie->visitGroup(visitEvent);
		
		if(visitEvent->isError()) {
			printf("Visit Event Error");
		}

		delete visitEvent; //will delete url as well
	}
}

void
PluginTest::loadIEPlugin()
{
	printf("loadIEPlugin() start\n");

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	wchar_t pluginDirectoryPath[1024];

	GetFullPathName(L"plugins\\Application_InternetExplorer.dll", 1024, pluginDirectoryPath, NULL);
	DebugPrint(L"Capture-Visitor: Plugin directory - %ls\n", pluginDirectoryPath);
	hFind = FindFirstFile(pluginDirectoryPath, &FindFileData);

	if (hFind != INVALID_HANDLE_VALUE) 
	{
		typedef void (*AppPlugin)(void*);
		std::wstring pluginDir = L"plugins\\";
		pluginDir += FindFileData.cFileName;			
		HMODULE hPlugin = LoadLibrary(pluginDir.c_str());

		if(hPlugin != NULL)
		{
			ie = createApplicationPluginObject(hPlugin);
			if(ie == NULL) {
				FreeLibrary(hPlugin);
			} else {
				printf("Loaded plugin: %ls\n", FindFileData.cFileName);
			}
		}
		FindClose(hFind);
	}
	printf("loadIEPlugin() end\n");
}

ApplicationPlugin*
PluginTest::createApplicationPluginObject(HMODULE hPlugin)
{
	DebugPrintTrace(L"Visitor::createApplicationPluginObject(HMODULE hPlugin) start\n");
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
		return applicationPlugin;
	} else {
		printf("Could not create application plugin");
	}
	DebugPrintTrace(L"Visitor::createApplicationPluginObject(HMODULE hPlugin) end\n");
	
}