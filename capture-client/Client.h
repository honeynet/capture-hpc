#pragma once
#include <string>
#include <tchar.h>
#include "IClient.h"


using namespace std;

class Client
{
public:
	Client(wstring n, wstring p, bool d, IClient *c)
	{
		name = n;
		path = p;
		clientVersion = L"";
		GetClientVersionInfo();
		printf("version: %ls %ls %ls %ls\n",name.c_str(), clientProductName.c_str(), path.c_str(), clientVersion.c_str());
		downloadURL = d;
		application = c;
	}
	~Client(void)
	{
	}
	wstring name;
	wstring path;
	wstring clientVersion;
	wstring clientProductName;
	bool downloadURL;
	IClient * application;
private:

	void GetClientVersionInfo()
	{
		struct LANGANDCODEPAGE {
			WORD wLanguage;
			WORD wCodePage;
		} *lpTranslate;
		DWORD dwHandle;
		DWORD nBufferSize;
		LPVOID pBuffer;
		LPTSTR pInfo;
		UINT nInfoSize;
		wchar_t sCharset[50];
		ZeroMemory(&sCharset, 50);
		wchar_t sQueryString[255];
		ZeroMemory(&sQueryString, 255);
		//printf("looking at: %ls\n", path.c_str());
		nBufferSize = GetFileVersionInfoSize(path.c_str(),&dwHandle);
		if (nBufferSize > 0)
		{
			pBuffer = (wchar_t *) malloc (nBufferSize);
			if (pBuffer != NULL)
			{
				if (GetFileVersionInfo(path.c_str(), dwHandle, nBufferSize, pBuffer))
				{
					VerQueryValue(pBuffer,L"\\VarFileInfo\\Translation",(LPVOID*)&lpTranslate,
							(UINT*)&nInfoSize);
					wsprintf(sCharset,L"%04x%04x", lpTranslate[0].wLanguage,
							lpTranslate[0].wCodePage);
					_tcscat_s(sQueryString, 255, L"\\StringFileInfo\\");
					_tcscat_s(sQueryString, 255, sCharset);
					_tcscat_s(sQueryString, 255, L"\\ProductVersion");
					VerQueryValue(pBuffer, sQueryString, (LPVOID*)&pInfo,
						(UINT*)&nInfoSize);
					clientVersion = (LPCWSTR)pInfo;

					ZeroMemory(&sQueryString, 255);
					_tcscat_s(sQueryString, 255, L"\\StringFileInfo\\");
					_tcscat_s(sQueryString, 255, sCharset);
					_tcscat_s(sQueryString, 255, L"\\ProductName");
					VerQueryValue(pBuffer, sQueryString, (LPVOID*)&pInfo,
						(UINT*)&nInfoSize);
					clientProductName = (LPCWSTR)pInfo;
					//printf("Added: %ls -> %ls\n", clientProductName.c_str(), clientVersion.c_str());
				}
				free(pBuffer);
			}
		}
	}
};
typedef pair <wstring, Client*> Client_Pair;

