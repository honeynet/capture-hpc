#include "InternetExplorerInstance.h"

InternetExplorerInstance::InternetExplorerInstance(void)
{
	major = 0;
	minor = 0;
}

InternetExplorerInstance::~InternetExplorerInstance(void)
{
		HWND hwndIE;
	DWORD dProcessID;
	pInternetExplorer->get_HWND((SHANDLE_PTR*)&hwndIE);
	GetWindowThreadProcessId(hwndIE, &dProcessID);

	HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, dProcessID);
	if(hProc != NULL)
	{
		if(!TerminateProcess(hProc, 0))
		{
			major = CAPTURE_VISITATION_PROCESS_ERROR;
			minor = GetLastError();
		}
	} else {
		major = CAPTURE_VISITATION_PROCESS_ERROR;
		minor = GetLastError();
	}

	//CloseHandle(hVisiting);
	pInternetExplorer->Release();

	CoUninitialize();
}

void 
InternetExplorerInstance::visitUrl(Url* url, HANDLE hEvent)
{
	HRESULT hr;
	hVisiting = hEvent;
	dwNetworkErrorCode = 0;
	major = CAPTURE_VISITATION_OK;
	CoInitializeEx(NULL,COINIT_MULTITHREADED);
	mainURL.vt = VT_EMPTY;
	hr = CoCreateInstance(CLSID_InternetExplorer,
		NULL,
		CLSCTX_SERVER,
		IID_IWebBrowser2,
		(void**)&pInternetExplorer );

	/* Create an IE window and connect this object to it so it can receive
	   events from IE */
	if ( SUCCEEDED ( hr ) )
	{
		//hVisiting = CreateEvent(NULL, FALSE, FALSE, NULL);
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
				hr = pConnectionPoint->Advise((IUnknown*)this,                              
					&m_dwConnectionToken);
			}
		}
	}
	
	/* Make the IE window just created visit a url */
	bNetworkError = false;
	pInternetExplorer->put_Visible(TRUE);
	_variant_t URL, Flag, TargetFrameName, PostData, Headers;
	URL = url->getUrl().c_str();
		
	hr = pInternetExplorer->Navigate2(&URL,&Flag,&TargetFrameName,&PostData,&Headers);
}

HRESULT STDMETHODCALLTYPE
InternetExplorerInstance::Invoke( 
									 DISPID dispIdMember,
									 REFIID riid,
									 LCID lcid,
									 WORD wFlags,
									 DISPPARAMS *pDispParams,
									 VARIANT *pVarResult,
									 EXCEPINFO *pExcepInfo,
									 UINT *puArgErr)
 {
	VARIANT * vt_statuscode;
	VARIANT * url;
    switch (dispIdMember)
    {
	case DISPID_BEFORENAVIGATE2:
		/* Put the first url (the main one) into a variable */
		if(mainURL.vt == VT_EMPTY)
		{
			mainURL = pDispParams->rgvarg[5].pvarVal;
		}
		break;
	case DISPID_NAVIGATECOMPLETE2:		
		break;
	case DISPID_DOCUMENTCOMPLETE:
		SetEvent(hVisiting);
		break;
	case DISPID_NAVIGATEERROR:		
		url = pDispParams->rgvarg[3].pvarVal;		
		HRESULT result;
		/* Compare the main url stored to the one this error msg is for
		   If they are not equal than ignore the error. This is because
		   a frame inside the main url has caused an error but we just
		   ignore it ... */
		if((result = VarBstrCmp(  
			url->bstrVal,         
			mainURL.bstrVal,  
			LOCALE_USER_DEFAULT,
			0)) == VARCMP_EQ)
		{
			vt_statuscode = pDispParams->rgvarg[1].pvarVal;
			dwNetworkErrorCode = vt_statuscode->lVal;
			major = CAPTURE_VISITATION_NETWORK_ERROR;
			minor = dwNetworkErrorCode;
			SetEvent(hVisiting);
		}
		break;
	default:
		break;
	}
	return(DISP_E_UNKNOWNINTERFACE);
 }
