/*
 *	PROJECT: Capture
 *	FILE: Client_OpenOffice.cpp
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
#include "Client_OpenOffice.h"

extern "C"  { 
	__declspec(dllexport) HRESULT New(IClient ** pClient, stdext::hash_map<wstring, Client*>* visitorClientsMap)
    {
		if(!*pClient)
		{
			*pClient= (IClient*) new Client_OpenOffice(visitorClientsMap);
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


Client_OpenOffice::Client_OpenOffice(stdext::hash_map<wstring, Client*>* visitorClientsMap)
{
	wstring path;

	GetCOMServerExecutablePath(L"com.sun.star.ServiceManager", &path);
	Client* word = new Client(L"oowriter", path, true, this);
	(*visitorClientsMap).insert(Client_Pair(L"oowriter", word));

	//GetCOMServerExecutablePath(L"Excel.Application", &path);
	//Client* excel = new Client(L"excel", path, false, this);
	//(*visitorClientsMap).insert(Client_Pair(L"excel", excel));

	//GetCOMServerExecutablePath(L"PowerPoint.Application", &path);
	//Client* powerpoint = new Client(L"powerpoint", path, false, this);
	//(*visitorClientsMap).insert(Client_Pair(L"powerpoint", powerpoint));
}

Client_OpenOffice::~Client_OpenOffice(void)
{

}

DWORD
Client_OpenOffice::Open(std::wstring client)
{
	HRESULT hr;
	CLSID clsOpenOffice;
	OLEobjectName = client;

	CoInitializeEx(NULL,COINIT_MULTITHREADED);

	hr = CLSIDFromProgID(L"com.sun.star.ServiceManager", &clsOpenOffice);
	if(FAILED(hr)) {
		return COM_FINDPROGID_ERROR;
	}
	hr = CoCreateInstance(clsOpenOffice, NULL, CLSCTX_LOCAL_SERVER,
			IID_IDispatch, (void **)&pOpenOffice);
	if(FAILED(hr)) {
		return COM_CREATEINSTANCE_ERROR;
	}
}

DWORD
Client_OpenOffice::Visit(std::wstring url, int visitTime, DWORD* minorErrorCode)
{
	VARIANT result;
	VariantInit(&result);

	VARIANT param1;
	VariantInit(&param1);
	param1.vt = VT_BSTR;
	param1.bstrVal = ::SysAllocString( L"com.sun.star.frame.Desktop");

	AutoWrap(DISPATCH_METHOD, &result, pOpenOffice, L"createInstance", 1, param1);

	pdispDesktop= result.pdispVal;

	pdispDesktop->AddRef(); 

	VARIANT resultDoc;
	VariantInit(&resultDoc);

	VARIANT parm[4];
	VariantInit(&parm[0]);
	parm[0].vt = VT_BSTR;
	wchar_t szUrlEncodedPath[1024];
	DWORD sUrl = 1024;

	UrlCreateFromPath(url.c_str(), szUrlEncodedPath, &sUrl, NULL);
	//UrlEscape(blah, blah, &sBlah, NULL);
	//printf("Escaped: %ls\n", blah);
	//parm[0].bstrVal = ::SysAllocString(L"private:factory/swriter");
	parm[0].bstrVal = ::SysAllocString(szUrlEncodedPath);

	VariantInit(&parm[1]);
	parm[1].vt = VT_BSTR;
	parm[1].bstrVal = ::SysAllocString(L"_frame");

	VariantInit(&parm[2]);
	parm[2].vt = VT_I4;
	parm[2].lVal = 0;

	
	VariantInit(&parm[3]);
	parm[3].vt = VT_ARRAY | VT_VARIANT;
	parm[3].parray = NULL; 

	HRESULT hr = AutoWrap(DISPATCH_METHOD, &resultDoc, pdispDesktop, L"loadComponentFromURL", 4, parm[3], parm[2], parm[1], parm[0]);

	pdispDoc= resultDoc.pdispVal; 
	Sleep(visitTime*1000);
	return hr;
}

DWORD
Client_OpenOffice::Close()
{

	VARIANT result;
	VariantInit(&result);

	AutoWrap(DISPATCH_METHOD, &result,  pdispDesktop, L"terminate", 0, NULL);

	pOpenOffice->Release();

	CoUninitialize();

	return 0;
}
