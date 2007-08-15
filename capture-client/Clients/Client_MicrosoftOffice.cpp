/*
 *	PROJECT: Capture
 *	FILE: Client_MicrosoftOffice.cpp
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
#include "Client_MicrosoftOffice.h"

extern "C"  { 
	__declspec(dllexport) HRESULT New(IClient ** pClient, stdext::hash_map<wstring, Client*>* visitorClientsMap)
    {
		if(!*pClient)
		{
			*pClient= (IClient*) new Client_MicrosoftOffice(visitorClientsMap);
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


Client_MicrosoftOffice::Client_MicrosoftOffice(stdext::hash_map<wstring, Client*>* visitorClientsMap)
{
	wstring path;

	GetCOMServerExecutablePath(L"Word.Application", &path);
	Client* word = new Client(L"word", path, false, this);
	(*visitorClientsMap).insert(Client_Pair(L"word", word));

	GetCOMServerExecutablePath(L"Excel.Application", &path);
	Client* excel = new Client(L"excel", path, false, this);
	(*visitorClientsMap).insert(Client_Pair(L"excel", excel));

	GetCOMServerExecutablePath(L"PowerPoint.Application", &path);
	Client* powerpoint = new Client(L"powerpoint", path, false, this);
	(*visitorClientsMap).insert(Client_Pair(L"powerpoint", powerpoint));
}

Client_MicrosoftOffice::~Client_MicrosoftOffice(void)
{

}

DWORD
Client_MicrosoftOffice::Open(std::wstring client)
{
	HRESULT hr;
	CLSID clsOfficeApplication;
	OLEobjectName = L"";
	

	if(client == L"word") {
		OLEobjectName = L"Word.Application";
	} else if(client == L"excel") {
		OLEobjectName = L"Excel.Application";
	} else if(client == L"powerpoint") {
		OLEobjectName = L"PowerPoint.Application";
	}
	wstring OLEobjectDocument = OLEobjectName;
	OLEobjectDocument = L".Documents";

	CoInitializeEx(NULL,COINIT_MULTITHREADED);

	hr = CLSIDFromProgID(OLEobjectName.c_str(), &clsOfficeApplication);
	if(FAILED(hr)) {
		return COM_FINDPROGID_ERROR;
	}
	hr = CoCreateInstance(clsOfficeApplication, NULL, CLSCTX_LOCAL_SERVER,
			IID_IDispatch, (void **)&pOfficeApplication);
	if(FAILED(hr)) {
		return COM_CREATEINSTANCE_ERROR;
	}
}

DWORD
Client_MicrosoftOffice::Visit(std::wstring url, int visitTime, DWORD* minorErrorCode)
{
	HRESULT hr;
	VARIANT result;
	unsigned int error;
	EXCEPINFO ExcepInfo;
	DISPID dispIDVisible;
	DISPID dispIDOpen;
	DISPID dispIDDocuments;
	OLECHAR *propertyName;
	if(OLEobjectName == L"PowerPoint.Application")
	{
		propertyName = L"Presentations";
	} else {
		propertyName = L"Documents";
	}
	
	DISPPARAMS documentsDispParams = { NULL, NULL, 0, 0 };
	hr = pOfficeApplication->GetIDsOfNames(IID_NULL, &propertyName, 1, LOCALE_SYSTEM_DEFAULT, &dispIDDocuments);
	if(FAILED(hr)) {
		return COM_GETIDOFNAME_ERROR;
	}
	hr = pOfficeApplication->Invoke(dispIDDocuments, IID_NULL, LOCALE_SYSTEM_DEFAULT,
			DISPATCH_PROPERTYGET,
			&documentsDispParams, &result, NULL, &error
			);
	if(FAILED(hr)) {
		return COM_INVOKE_ERROR;
	}
	pOfficeApplicationDocument = result.pdispVal;

	propertyName = L"Visible";
	DISPPARAMS visibleDispParams = { NULL, NULL, 0, 0 };
	hr = pOfficeApplication->GetIDsOfNames(IID_NULL, &propertyName, 1, LOCALE_SYSTEM_DEFAULT, &dispIDVisible);
	if(FAILED(hr)) {
		return COM_GETIDOFNAME_ERROR;
	}

	VARIANT varTrue;
	varTrue.vt = VT_BOOL;
	varTrue.boolVal = 1;
	visibleDispParams.cArgs = 1;
	visibleDispParams.rgvarg = &varTrue;
	visibleDispParams.cNamedArgs = 1;
	DISPID dispidNamed = DISPID_PROPERTYPUT;
	visibleDispParams.rgdispidNamedArgs = &dispidNamed;

	hr = pOfficeApplication->Invoke(dispIDVisible, IID_NULL, LOCALE_SYSTEM_DEFAULT,
			DISPATCH_PROPERTYPUT | DISPATCH_METHOD,
			&visibleDispParams, &result, NULL, &error
			);
	if(FAILED(hr)) {
		return COM_INVOKE_ERROR;
	}

	propertyName = L"Open";
	DISPPARAMS openDispParams = { NULL, NULL, 0, 0 };
	hr = pOfficeApplicationDocument->GetIDsOfNames(IID_NULL, &propertyName, 1, LOCALE_SYSTEM_DEFAULT, &dispIDOpen);
	if(FAILED(hr)) {
		return COM_GETIDOFNAME_ERROR;
	}

	VARIANT varTrue2;
	varTrue2.vt = VT_BSTR;
	varTrue2.bstrVal = _bstr_t(url.c_str());
	openDispParams.cArgs = 1;
	openDispParams.rgvarg = &varTrue2;

	hr = pOfficeApplicationDocument->Invoke(dispIDOpen, IID_NULL, LOCALE_SYSTEM_DEFAULT,
			DISPATCH_PROPERTYPUT | DISPATCH_METHOD,
			&openDispParams, &result, &ExcepInfo, &error
			);
	if(FAILED(hr)) {
		return COM_INVOKE_ERROR;
	}

	Sleep(visitTime*1000);

	//OfficeApplication->Invoke();
}

DWORD
Client_MicrosoftOffice::Close()
{
	unsigned int error;
	HRESULT hr;
	VARIANT result;
	DISPID dispIDExit;
	DISPPARAMS exitDispParams = { NULL, NULL, 0, 0 };

	OLECHAR *propertyName = L"Quit";
	hr = pOfficeApplication->GetIDsOfNames(IID_NULL, &propertyName, 1,
                                         LOCALE_USER_DEFAULT, &dispIDExit);
	if(FAILED(hr)) {
		return COM_GETIDOFNAME_ERROR;
	}
	hr = pOfficeApplication->Invoke(
			dispIDExit, IID_NULL, LOCALE_SYSTEM_DEFAULT,
			DISPATCH_METHOD,
			&exitDispParams, &result, NULL, &error
		);
	if(FAILED(hr)) {
		return COM_INVOKE_ERROR;
	}

	pOfficeApplication->Release();
	pOfficeApplicationDocument->Release();

	CoUninitialize();
}
