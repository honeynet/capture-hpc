/*
 *	PROJECT: Capture
 *	FILE: Client_InternetExplorer.h
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
#pragma once
#define _WIN32_WINNT 0x0501
#include "..\Client.h"
#include "ComHelper.h"
#include <Exdisp.h>
#include <comutil.h>
#include <exdispid.h>
#include <objbase.h>
#include <hash_map>

using namespace std;

class Client_InternetExplorer : DWebBrowserEvents2, IClient
{
public:
	Client_InternetExplorer(stdext::hash_map<wstring, Client*>* visitorClientsMap);
	~Client_InternetExplorer(void);

	/* IClient interface */
	DWORD Open(std::wstring client);
	DWORD Visit(std::wstring url, int visitTime, DWORD* minorErrorCode);
	DWORD Close();

	HRESULT STDMETHODCALLTYPE QueryInterface( 
                /* [in] */ REFIID riid,
                /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, DIID_DWebBrowserEvents2))
		{
			*ppvObject = this;
			return(NOERROR);
		}
		*ppvObject = 0;
		return(E_NOINTERFACE);
	}
	ULONG STDMETHODCALLTYPE AddRef( void)
	{
		return(1);
	}
            
    ULONG STDMETHODCALLTYPE Release( void)
	{
		return(1);
	}
	HRESULT STDMETHODCALLTYPE GetTypeInfoCount( 
            /* [out] */ UINT *pctinfo)
	{
		return(NOERROR);
	}
        
    HRESULT STDMETHODCALLTYPE GetTypeInfo( 
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo)
	{
		return(NOERROR);
	}
        
    HRESULT STDMETHODCALLTYPE GetIDsOfNames( 
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId)
	{
		return(NOERROR);
	}
        
     HRESULT STDMETHODCALLTYPE Invoke( 
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr)
	 {
		VARIANT * vt_statuscode;
        switch (dispIdMember)
        {
           case DISPID_NAVIGATECOMPLETE2: 
				//printf("DISPID_NAVIGATECOMPLETE2\n");			
				break;
		   case DISPID_DOCUMENTCOMPLETE:
				//printf("DISPID_DOCUMENTCOMPLETE\n");
				SetEvent(hVisiting);
				break;
		   case DISPID_NAVIGATEERROR:
				vt_statuscode = pDispParams->rgvarg[1].pvarVal;
				dwNetworkErrorCode = vt_statuscode->lVal;
				bNetworkError = true;
				SetEvent(hVisiting);
				break;
		   default:
			   //printf("UNKOWN: %i\n",dispIdMember);
			   break;
		}
		return(DISP_E_UNKNOWNINTERFACE);
	 }

	
	HANDLE hVisiting;
	bool bNetworkError;
	DWORD dwNetworkErrorCode;
private:
	IWebBrowser2* pInternetExplorer;
	bool open;
};
