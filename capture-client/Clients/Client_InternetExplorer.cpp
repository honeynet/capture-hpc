/*
 *	PROJECT: Capture
 *	FILE: Client_InternetExplorer.cpp
 *	AUTHORS: Ramon Steenson (rsteenson@gmail.com) & Christian Seifert (christian.seifert@gmail.com)
 *
 *	Developed by Victoria University of Wellington and the New Zealand Honeynet Alliance
 *
 *	This file is part of Capture.
 *
 *	Capture is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Capture is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Capture; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "Client_InternetExplorer.h"

extern "C"  { 
	__declspec(dllexport) HRESULT New(IClient ** pClient, stdext::hash_map<wstring, Client*>* visitorClientsMap)
    {
		if(!*pClient)
		{
			*pClient= (IClient*) new Client_InternetExplorer(visitorClientsMap);
			return S_OK;
		}
		return E_FAIL;
    }

	__declspec(dllexport) HRESULT Delete(IClient ** pClient)
    {
		if(!*pClient)
			return E_FAIL;
		delete *pClient;
		*pClient = 0;
		return S_OK;
    }

};

Client_InternetExplorer::Client_InternetExplorer(stdext::hash_map<wstring, Client*>* visitorClientsMap)
{
	wstring path;

	GetCOMServerExecutablePath(L"InternetExplorer.Application", &path);
	Client* iexplore = new Client(L"iexplore", path, false, this);
	(*visitorClientsMap).insert(Client_Pair(L"iexplore", iexplore));	
}

Client_InternetExplorer::~Client_InternetExplorer(void)
{
}

DWORD
Client_InternetExplorer::Open(std::wstring client)
{
	HRESULT     hr;

	CoInitializeEx(NULL,COINIT_MULTITHREADED);

	hr = CoCreateInstance ( CLSID_InternetExplorer,         // CLSID of coclass
                        NULL,                    // not used - aggregation
                        CLSCTX_SERVER,    // type of server
                        IID_IWebBrowser2,          // IID of interface
                        (void**) &pInternetExplorer );        // Pointer to our interface pointer

	if ( SUCCEEDED ( hr ) )
	{
		hVisiting = CreateEvent(NULL, FALSE, FALSE, NULL);
		IConnectionPointContainer *pCPContainer; 
		if(SUCCEEDED(pInternetExplorer->QueryInterface(IID_IConnectionPointContainer,
			(void **)&pCPContainer)))
		{
			IConnectionPoint *pConnectionPoint;

			hr = pCPContainer->FindConnectionPoint(DIID_DWebBrowserEvents2,                      
				&pConnectionPoint);

			if(SUCCEEDED(hr))
			{
				DWORD m_dwConnectionToken;
				hr = pConnectionPoint->Advise((IDispatch*)this,                              
					&m_dwConnectionToken);
			} else {
				//printf("failed2\n");
			}
		} else {
			//printf("failed3\n");
		}
	} else {
		//printf("failed1\n");
	}
	return 0;
}

DWORD
Client_InternetExplorer::Visit(std::wstring url, int visitTime, DWORD* minorErrorCode)
{
	bNetworkError = false;
	pInternetExplorer->put_Visible(TRUE);
	_variant_t URL, Flag, TargetFrameName, PostData, Headers;
	URL = url.c_str();
		
	HRESULT hr = pInternetExplorer->Navigate2(&URL,&Flag,&TargetFrameName,&PostData,&Headers);
		
	DWORD dResult = WaitForSingleObject(hVisiting, 60*1000);

	if(dResult == WAIT_TIMEOUT)
	{
		return VISIT_TIMEOUT_ERROR;
	} else {
		Sleep(visitTime*1000);
	}
	if(bNetworkError)
	{
		*minorErrorCode = dwNetworkErrorCode;
		return VISIT_NETWORK_ERROR;
	}
	return VISIT_FINISH;
}

DWORD 
Client_InternetExplorer::Close()
{
	HWND hwndIE;
	DWORD dProcessID;
	pInternetExplorer->get_HWND((SHANDLE_PTR*)&hwndIE);
	GetWindowThreadProcessId(hwndIE, &dProcessID);

	HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, dProcessID);
		
	//if(pInternetExplorer->Quit() != S_OK)
	if(!TerminateProcess(hProc, 0)) {
			printf("ERROR - Cannot terminate IE process: %x\n", GetLastError());
	}
	CloseHandle(hVisiting);
	pInternetExplorer->Release();

	CoUninitialize();
	return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    return TRUE;
}