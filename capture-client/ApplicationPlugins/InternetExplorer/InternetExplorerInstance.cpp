#include "InternetExplorerInstance.h"

#define _WIN32_WINNT 0x0501
#define _WIN32_DCOM
#include <objbase.h>

InternetExplorerInstance::InternetExplorerInstance(IClassFactory* ie_factory)
{
	DebugPrintTrace(L"InternetExplorerInstance::InternetExplorerInstance(IClassFactory* ie_factory) start\n");
	internet_explorer_factory = ie_factory;
	major = 0;
	minor = 0;
	DebugPrintTrace(L"InternetExplorerInstance::InternetExplorerInstance(IClassFactory* ie_factory) end\n");
}

InternetExplorerInstance::~InternetExplorerInstance(void)
{
	DebugPrintTrace(L"InternetExplorerInstance::~InternetExplorerInstance(void) start\n");

	DebugPrintTrace(L"InternetExplorerInstance::~InternetExplorerInstance(void) end\n");
}


void 
InternetExplorerInstance::visitUrl(Url* url)
{
	DebugPrintTrace(L"InternetExplorerInstance::visitUrl(Url* url) start\n");
	HRESULT hr;
	hVisiting = CreateEvent(NULL, false, NULL, NULL);
	dwNetworkErrorCode = 0;
	major = CAPTURE_VISITATION_OK;
	CoInitializeEx(NULL,COINIT_MULTITHREADED);
	mainURL.vt = VT_EMPTY;
	exited = false;

	int blah = GetTickCount() % 1024;
	
	hr = internet_explorer_factory->CreateInstance(NULL, IID_IWebBrowser2, 
							(void**)&pInternetExplorer);
	
	/* Create an IE window and connect this object to it so it can receive
	   events from IE */
	if ( SUCCEEDED ( hr ) )
	{
		HWND hwndIE;
		DWORD dProcessId;
		pInternetExplorer->get_HWND((SHANDLE_PTR*)&hwndIE);
		
		GetWindowThreadProcessId(hwndIE, &dProcessId);
		

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

				/* Make the IE window just created visit a url */
				bNetworkError = false;
				bool wait = true;
				pInternetExplorer->put_Visible(TRUE);
				_variant_t URL, Flag, TargetFrameName, PostData, Headers;
				URL = url->getUrl().c_str();

				hr = pInternetExplorer->Navigate2(&URL,&Flag,&TargetFrameName,&PostData,&Headers);
				
				// Wait for the IE instance to visit the url
				DWORD dwWait = WaitForSingleObject(hVisiting, 180*1000); //determines timeout error
				
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
				bool closed = Close();

				if( !closed )
				{
					url->setMajorErrorCode( CAPTURE_VISITATION_WARNING );
					url->setMinorErrorCode( CAPTURE_PE_PROCESS_ALREADY_TERMINATED );
				}
			}
		}
	} else {
		printf("Error: Unable to create IE window.\n");
		url->setMajorErrorCode(CAPTURE_VISITATION_PROCESS_ERROR);
	}
	
	
	DebugPrintTrace(L"InternetExplorerInstance::visitUrl(Url* url) end\n");
}

bool
InternetExplorerInstance::Close()
{
	DebugPrintTrace(L"InternetExplorerInstance::Close() start\n");
	// Closes the IE window, except for the last one
	HRESULT hr = pInternetExplorer->Quit();

	ULONG num_references = pInternetExplorer->Release();

	CloseHandle(hVisiting);
	CoUninitialize();

	DebugPrintTrace(L"InternetExplorerInstance::Close() end\n");
	return (hr == S_OK);
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
	DebugPrintTrace(L"IInternetExplorerInstance::Invoke(...) start\n");
	VARIANT * vt_statuscode;
	VARIANT * url;
    switch (dispIdMember)
    {
	case DISPID_BEFORENAVIGATE2:
		DebugPrint(L"BeforeNavigate2");
		/* Put the first url (the main one) into a variable */
		if(mainURL.vt == VT_EMPTY)
		{
			mainURL = pDispParams->rgvarg[5].pvarVal;
		}
		
		break;
	case DISPID_NAVIGATECOMPLETE2:		
		DebugPrint(L"NavigateComplete2");
		break;
	case 283:
		DebugPrint(L"DocumentComplete");
		SetEvent(hVisiting);
		break;
	case DISPID_DOCUMENTCOMPLETE:
		DebugPrint(L"DocumentComplete");
		SetEvent(hVisiting);
		break;
	case DISPID_NAVIGATEERROR:		
		DebugPrint(L"NavigateError");
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
	case DISPID_ONQUIT:
		DebugPrint(L"onQuit");
		exited = true;
		SetEvent(hVisiting);
		break;
	default:
		DebugPrint(L"default");
		break;
	}
	DebugPrintTrace(L"IInternetExplorerInstance::Invoke(...) end\n");
	return(DISP_E_UNKNOWNINTERFACE);
 }
