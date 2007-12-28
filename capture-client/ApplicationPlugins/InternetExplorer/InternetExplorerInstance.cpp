#include "InternetExplorerInstance.h"

InternetExplorerInstance::InternetExplorerInstance(IClassFactory* ie_factory)
{
	internet_explorer_factory = ie_factory;
	major = 0;
	minor = 0;
}

InternetExplorerInstance::~InternetExplorerInstance(void)
{

}

void 
InternetExplorerInstance::visitUrl(Url* url)
{
	HRESULT hr;
	hVisiting = CreateEvent(NULL, false, NULL, NULL);
	dwNetworkErrorCode = 0;
	major = CAPTURE_VISITATION_OK;
	CoInitializeEx(NULL,COINIT_MULTITHREADED);
	mainURL.vt = VT_EMPTY;

	int blah = GetTickCount() % 1024;
	
	//printf("sleeping: %i\n", blah);
	//Sleep(blah);
	hr = internet_explorer_factory->CreateInstance(NULL, IID_IWebBrowser2, 
							(void**)&pInternetExplorer);
	//internet_explorer_factory->Release();
	//hr = CoCreateInstance(CLSID_InternetExplorer,
	//	NULL,
	//	CLSCTX_SERVER,
	//	IID_IWebBrowser2,
	//	(void**)&pInternetExplorer );

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
	} else {
		_ASSERT(false);
	}
	
	/* Make the IE window just created visit a url */
	bNetworkError = false;
	bool wait = true;
	pInternetExplorer->put_Visible(TRUE);
	_variant_t URL, Flag, TargetFrameName, PostData, Headers;
	URL = url->getUrl().c_str();
		
	hr = pInternetExplorer->Navigate2(&URL,&Flag,&TargetFrameName,&PostData,&Headers);
	
	// Wait for the IE instance to visit the url
	DWORD dwWait = WaitForSingleObject(hVisiting, 600000);
	
	if(dwWait == WAIT_TIMEOUT)
	{
		url->setMajorErrorCode(CAPTURE_VISITATION_TIMEOUT_ERROR);
		wait = false;
	} else {
		url->setMajorErrorCode(major);
		url->setMinorErrorCode(minor);

		if(minor >= 0x800C0000 && minor <= 0x800CFFFF) {
			wait = false;
		} else {
			url->setVisited(true);
		}
	}

	// If it has visited the site and no error has occured wait the required time set by the url
	if(wait)
	{
		Sleep(url->getVisitTime()*1000);
	}

	// Close the IE window
	Close();
}

void
InternetExplorerInstance::Close()
{
	// Closes the IE window, except for the last one
	HRESULT hr = pInternetExplorer->Quit();

	pInternetExplorer->Release();

	CloseHandle(hVisiting);
	CoUninitialize();
	
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
