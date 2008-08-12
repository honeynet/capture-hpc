#include "Application_Safari.h"
#include <windows.h>
#include <shlobj.h>
#include <string>
#include <iostream>
#include <fstream>


Application_Safari::Application_Safari(void)
{
}

Application_Safari::~Application_Safari(void)
{
}



void
Application_Safari::visitGroup(VisitEvent* visitEvent)
{
	unsigned int numUrls = visitEvent->getUrls().size();
	if(numUrls > 1)
	{
		printf("Application_Safari: WARNING - Visiting multiple urls at the same time is");
		printf(" currently not supported by this plugin. Visiting urls one-by-one\n");
	}
	PROCESS_INFORMATION* piProcessInfo = new PROCESS_INFORMATION[numUrls];
	bool wait = true;
	for(int i = 0; i < numUrls; i++)
	{
		Url* url = visitEvent->getUrls()[i];
		DWORD result = visitUrl(url, &piProcessInfo[i]);
		if(result != SUCCESS)
		{
			visitEvent->setErrorCode(result);
			wait = false;
		}
	}

	//for(int i = 0; i < numUrls; i++)
	//{
	//	DWORD minorError = 0;
	//	Url* url = visitEvent->getUrls()[i];
	//	if(!TerminateProcess(piProcessInfo[i].hProcess, 0))
	//	{
	//		visitEvent->setErrorCode(CAPTURE_VISITATION_PROCESS_ERROR);
	//		url->setMajorErrorCode(CAPTURE_VISITATION_PROCESS_ERROR);
	//		url->setMinorErrorCode(GetLastError());
	//	}
	//}
	delete [] piProcessInfo;
}
	
void Application_Safari::setHomePage(wchar_t* userDataPath, std::wstring safariPath, std::wstring url) {
	
	
		safariPath = userDataPath + safariPath;

	
	std::wstring document;
	bool opened = openDocument(safariPath, document);
	if(!opened)
	{
		// If file was not opened use a blank preferences file and go from there ...
		document = safariBlankPreferences;
	}
	

	const std::wstring startString = L"<string>";
	const std::wstring endString = L"</string>";
	const std::wstring homepageTag = L"<key>HomePage</key>";

	// Find where the homepage key and value start
	size_t homepageKey = document.find(homepageTag);
	if(homepageKey != std::wstring::npos)
	{
		size_t homepageValueStart = document.find(startString, homepageKey);
		size_t homepageValueEnd = document.find(endString, homepageValueStart);
		
		// Replace the string value with the url
		// TODO probably need to at least XML escape the url
		if(homepageValueStart != std::wstring::npos && homepageValueEnd != std::wstring::npos)
		{
			document.replace(homepageValueStart+startString.length(), homepageValueEnd-homepageValueStart-startString.length(), url);
		}
	} else {
		// Could not find the homepage key so add it
		const std::wstring lastTag = L"</dict>";
		
		size_t lastTagStart = document.find(lastTag);
		if(lastTagStart != std::wstring::npos)
		{
			std::wstring homepage = L"\t\t";
			homepage = homepageTag;
			homepage += L"\n\t\t<string>";
			homepage += url;
			homepage += L"</string>\n";
			document.insert(lastTagStart, homepage);
		} else {
			// Something is horribly wrong
		}
	}

	// Write the document back
	writeDocument(safariPath, document);
}

DWORD
Application_Safari::visitUrl(Url* url, PROCESS_INFORMATION* piProcessInfo)
{
	DWORD status = 0;
	wchar_t* userDataPath = new wchar_t[MAX_PATH];
	HRESULT result = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, userDataPath);

	if(result == S_OK)
	{
		setHomePage(userDataPath,L"\\Apple Computer\\Safari\\Preferences.plist",url->getUrl());
		setHomePage(userDataPath,L"\\Apple Computer\\Safari\\Preferences\\com.apple.Safari.plist",url->getUrl());
	}
	delete [] userDataPath;

	// Open the actual Safari process
	STARTUPINFO siStartupInfo;

	memset(&siStartupInfo, 0, sizeof(siStartupInfo));
	siStartupInfo.cb = sizeof(siStartupInfo);

	wstring processCommand = L"\"";
	processCommand += L"C:\\Program Files\\Safari\\Safari.exe";
	processCommand += L"\"";

	BOOL created = CreateProcess(NULL,(LPWSTR)processCommand.c_str(), 0, 0, FALSE,
		CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo,
		piProcessInfo);

	if(created == FALSE)
	{
		url->setMajorErrorCode(CAPTURE_VISITATION_PROCESS_ERROR);
		return GetLastError();
	} else {
		Sleep(url->getVisitTime()*1000);
	}

	if(!TerminateProcess(piProcessInfo->hProcess, 0))
	{
		url->setMajorErrorCode(CAPTURE_VISITATION_PROCESS_ERROR);
		url->setMinorErrorCode(GetLastError());
		return GetLastError();
	}
	
	return status;
}

bool
Application_Safari::openDocument(const wstring& file, std::wstring& document)
{
	std::wstring line;
	wifstream fileStream (file.c_str());
	if (fileStream.is_open())
	{
		while (!fileStream.eof())
		{
			getline(fileStream, line);
			document += line;	
			if(!fileStream.eof())
			{
				document += L"\n";
			}
		}
		fileStream.close();
		return true;
	} else {
		return false;
	}
}

void
Application_Safari::writeDocument(const wstring& file, const wstring& document)
{
	wofstream outputF (file.c_str());
	if (outputF.is_open())
	{
		outputF.write(document.c_str(), document.length());
	}
	outputF.close();
}