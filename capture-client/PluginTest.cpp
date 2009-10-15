#include "Precompiled.h"

#include "PluginTest.h"

PluginTest::PluginTest(void)
{
}

PluginTest::~PluginTest(void)
{
}

void PluginTest::loadTest() {
	loadIEBulkPlugin();

	for(int i=0;i<1;i++) {
		VisitEvent* visitEvent = new VisitEvent();
		visitEvent->setIdentifier(L"1");
		visitEvent->setProgram(L"iexplorebulk");


		for(int i = 0; i < 50; i++) {
			std::wstring urlStr = L"http://www.google.com/search?q=";
			urlStr += i;
			Url* url = new Url(urlStr,L"iexplorebulk",60);
			visitEvent->addUrl(url);
		}
		
		ie->visitGroup(visitEvent); 

		if(visitEvent->isError()) {
			
			LOG(INFO, "PLUGIN TEST: Visit Event Error %d", visitEvent->getErrorCode());
		}
		
		
		for each(Url* url in visitEvent->getUrls())
		{
			if(url->getMajorErrorCode()!=0) {
				LOG(INFO, "PLUGIN TEST: URL ERROR %d",url->getMajorErrorCode());
			}
		}


		delete visitEvent; //will delete url as well

	}
}

std::wstring
PluginTest::errorCodeToString(DWORD errorCode)
{
	wchar_t szTemp[16];
	swprintf_s(szTemp, 16, L"%08x", errorCode);
	std::wstring error = szTemp;
	return error;
}


void
PluginTest::loadIEPlugin()
{
	LOG(INFO, "loadIEPlugin() start\n");

	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	wchar_t pluginDirectoryPath[1024];

	GetFullPathName(L"plugins\\Application_InternetExplorer.dll", 1024, pluginDirectoryPath, NULL);
	LOG(INFO, "Visitor: Plugin directory - %ls", pluginDirectoryPath);
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
				LOG(INFO, "Loaded plugin: %ls", FindFileData.cFileName);
			}
		}
		FindClose(hFind);
	}
}

void
PluginTest::loadIEBulkPlugin()
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	wchar_t pluginDirectoryPath[1024];

	GetFullPathName(L"plugins\\Application_InternetExplorerBulk.dll", 1024, pluginDirectoryPath, NULL);
	LOG(INFO, "Visitor: Plugin directory - %ls", pluginDirectoryPath);
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
				LOG(INFO, "Loaded plugin: %ls", FindFileData.cFileName);
			}
		}
		FindClose(hFind);
	}
}

ApplicationPlugin*
PluginTest::createApplicationPluginObject(HMODULE hPlugin)
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
		return applicationPlugin;
	} else {
		LOG(INFO, "Could not create application plugin");
	}
	
}