#include "InternetExplorerInstanceBulk.h"

#define _WIN32_WINNT 0x0501
#define _WIN32_DCOM
#include <objbase.h>

InternetExplorerInstanceBulk::InternetExplorerInstanceBulk(std::wstring fullPathToExe)
{
	DebugPrintTrace(L"InternetExplorerInstanceBulk::InternetExplorerInstance(IClassFactory* ie_factory) start\n");
	m_fullPathToExe = fullPathToExe;
	
	// allocate mem for process info
	memset(&m_piProcessInfo, 0, sizeof(m_piProcessInfo));

	major = 0;
	minor = 0;
	DebugPrintTrace(L"InternetExplorerInstanceBulk::InternetExplorerInstance(IClassFactory* ie_factory) end\n");
}

InternetExplorerInstanceBulk::~InternetExplorerInstanceBulk(void)
{
	DebugPrintTrace(L"InternetExplorerInstanceBulk::~InternetExplorerInstance(void) start\n");
	CloseHandle(m_piProcessInfo.hProcess);
    CloseHandle(m_piProcessInfo.hThread);
	DebugPrintTrace(L"InternetExplorerInstanceBulk::~InternetExplorerInstance(void) end\n");
}


DWORD InternetExplorerInstanceBulk::executeProcess() 
{
	size_t iMyCounter = 0; 
	DWORD iReturnVal = 0; 
    DWORD dwExitCode = 0;
     
     /* CreateProcess API initialization */
     STARTUPINFOW siStartupInfo;
     memset(&siStartupInfo, 0, sizeof(siStartupInfo));
     siStartupInfo.cb = sizeof(siStartupInfo);

     if (!CreateProcessW(const_cast<LPCWSTR>(m_fullPathToExe.c_str()), NULL, 
                                  0, 0, true,
                                  CREATE_DEFAULT_ERROR_MODE, 0, 0,
                                  &siStartupInfo, &m_piProcessInfo))
     {
		/* CreateProcess failed */
		iReturnVal = GetLastError();
	 } else {
		 WaitForInputIdle(m_piProcessInfo.hProcess, 2000);
		 Sleep(1000);
	 }
     return iReturnVal;
}

DWORD InternetExplorerInstanceBulk::getProcessId(void) 
{
	return m_piProcessInfo.dwProcessId;
}


bool InternetExplorerInstanceBulk::setCurrentWebBrowserIF()
{

    HRESULT hr;
    SHDocVw::IShellWindowsPtr spSHWinds; 
    hr = spSHWinds.CreateInstance (__uuidof(SHDocVw::ShellWindows)); 
    
    if (FAILED (hr))
        return false;

    _ASSERT (spSHWinds != NULL);
	DebugPrint(L"InternetExplorerInstanceBulk::setCurrentWebBrowserIF created shellWindows\n");    
    
	long nCount = spSHWinds->GetCount ();

    IDispatchPtr spDisp;
    
    for (long i = 0; i < nCount; i++)
    {
		DebugPrint(L"InternetExplorerInstanceBulk::setCurrentWebBrowserIF iterating window\n");
        _variant_t va (i, VT_I4);
        spDisp = spSHWinds->Item (va);
        
        IWebBrowser2 * pWebBrowser = NULL;
        hr = spDisp.QueryInterface (IID_IWebBrowser2, & pWebBrowser);
        
        if (pWebBrowser != NULL)
        {
			DebugPrint(L"InternetExplorerInstanceBulk::setCurrentWebBrowserIF found browser window\n");
            HWND hwndIE;
			DWORD dProcessId;
			pWebBrowser->get_HWND((SHANDLE_PTR*)&hwndIE);
			GetWindowThreadProcessId(hwndIE, &dProcessId);

			//check pid of pWebBrowser
			if(dProcessId==m_piProcessInfo.dwProcessId) {
				DebugPrint(L"InternetExplorerInstanceBulk::setCurrentWebBrowserIF found our window\n");
				pInternetExplorer = pWebBrowser;
				return true;
			}
            pWebBrowser->Release ();
        }
    }
    
    return false;
}

void 
InternetExplorerInstanceBulk::visitUrl(Url* url)
{
	DebugPrintTrace(L"InternetExplorerInstanceBulk::visitUrl(Url* url) start\n");
	bool wait = true;
	HRESULT hr;
	DWORD iReturnVal = executeProcess();

	if(iReturnVal==0) 
	{
		//connect and register event handler
		if(setCurrentWebBrowserIF()) { //now pInternetExplorer is set
					
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

					url->setProcessId(getProcessId());

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
			printf("Error: Unable to find created IE window.\n");
			url->setMajorErrorCode(CAPTURE_VISITATION_PROCESS_ERROR);
		}
	
	} else {
		printf("Error: Unable to create IE window.\n");
		url->setMajorErrorCode(CAPTURE_VISITATION_PROCESS_ERROR);
	}


	DebugPrintTrace(L"InternetExplorerInstanceBulk::visitUrl(Url* url) end\n");
}

bool
InternetExplorerInstanceBulk::Close()
{
	DebugPrintTrace(L"InternetExplorerInstanceBulk::Close() start\n");
	
	//first the process denoted by this process handler
	EnumWindows(InternetExplorerInstanceBulk::EnumWindowsCloseProc, (LPARAM) this);
	//lets try to terminate
	if(!TerminateProcess(m_piProcessInfo.hProcess, 0))
	{
		return false;
	}

	DebugPrintTrace(L"InternetExplorerInstanceBulk::Close() end\n");
	return true;
}

BOOL CALLBACK InternetExplorerInstanceBulk::EnumWindowsCloseProc(HWND hwnd,LPARAM lParam)
{
	InternetExplorerInstanceBulk* internetExplorerInstanceBulk = (InternetExplorerInstanceBulk*) lParam;
	DWORD processId = internetExplorerInstanceBulk->getProcessId();
	if (GetWindowLong(hwnd,GWL_STYLE) & WS_VISIBLE) {
		DWORD windowsProcessId;
		GetWindowThreadProcessId(hwnd, &windowsProcessId);
		if (windowsProcessId == processId) 
		{
			SendMessage(hwnd, WM_CLOSE, 1, 0);		
		}
	}
	return TRUE;
}

HRESULT STDMETHODCALLTYPE
InternetExplorerInstanceBulk::Invoke( 
									 DISPID dispIdMember,
									 REFIID riid,
									 LCID lcid,
									 WORD wFlags,
									 DISPPARAMS *pDispParams,
									 VARIANT *pVarResult,
									 EXCEPINFO *pExcepInfo,
									 UINT *puArgErr)
 {
	DebugPrintTrace(L"InternetExplorerInstanceBulk::Invoke(...) start\n");
	printf("Dispatch Event ID: %d\n",dispIdMember);
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
	DebugPrintTrace(L"InternetExplorerInstanceBulk::Invoke(...) end\n");
	return(DISP_E_UNKNOWNINTERFACE);
 }
