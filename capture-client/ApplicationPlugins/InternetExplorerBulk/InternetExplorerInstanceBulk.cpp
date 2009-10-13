#include "InternetExplorerInstanceBulk.h"

#define _WIN32_WINNT 0x0501
#define _WIN32_DCOM
#include <objbase.h>

InternetExplorerInstanceBulk::InternetExplorerInstanceBulk(std::wstring fullPathToExe)
{
	m_fullPathToExe = fullPathToExe;
	
	ZeroMemory( &m_piProcessInfo, sizeof(m_piProcessInfo) );

	pInternetExplorer = NULL;
	major = 0;
	minor = 0;
}

InternetExplorerInstanceBulk::~InternetExplorerInstanceBulk(void)
{
	CloseHandle(m_piProcessInfo.hProcess);
    CloseHandle(m_piProcessInfo.hThread);
}


DWORD InternetExplorerInstanceBulk::executeProcess() 
{
	size_t iMyCounter = 0; 
	DWORD iReturnVal = 0; 
    DWORD dwExitCode = 0;
     
     /* CreateProcess API initialization */
     STARTUPINFOW siStartupInfo;
     ZeroMemory( &siStartupInfo, sizeof(siStartupInfo) );
     siStartupInfo.cb = sizeof(siStartupInfo);



     if (!CreateProcessW(const_cast<LPCWSTR>(m_fullPathToExe.c_str()), NULL, 
                                  0, 0, true,
                                  CREATE_DEFAULT_ERROR_MODE, 0, 0,
                                  &siStartupInfo, &m_piProcessInfo))
     {
		/* CreateProcess failed */
		iReturnVal = GetLastError();
	 } else {
		WaitForInputIdle(m_piProcessInfo.hProcess, 20000);
		double maxWaitTimeInSec = 20;
		double waitTimeInSec = 0;

		bool isOpen = this->isOpen();
		while(!isOpen && waitTimeInSec < maxWaitTimeInSec) 
		{
			Sleep(100);
			waitTimeInSec = waitTimeInSec + 0.1;
			isOpen = this->isOpen();
		}
		if(waitTimeInSec >= maxWaitTimeInSec) {
			iReturnVal = -5;
		} else {
		}

		
	 }
     return iReturnVal;
}

DWORD InternetExplorerInstanceBulk::getProcessId(void) 
{
	return m_piProcessInfo.dwProcessId;
}

bool InternetExplorerInstanceBulk::isOpen()
{
	bool open = false;

	DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
        return false;

    // Calculate how many process identifiers were returned.

    cProcesses = cbNeeded / sizeof(DWORD);

    for ( i = 0; i < cProcesses; i++ )
	{
        if( aProcesses[i] != 0 )
		{
			if(aProcesses[i]==this->getProcessId()) 
			{
				//try to attach to browser; then we consider to be open
				if(setCurrentWebBrowserIF() && pInternetExplorer!=NULL) {
					open = true;
				}
				
			}

		}
	}
	return open;
}

bool InternetExplorerInstanceBulk::setCurrentWebBrowserIF()
{
	CoInitializeEx(NULL,COINIT_MULTITHREADED);
    HRESULT hr;
    SHDocVw::IShellWindowsPtr spSHWinds; 
    hr = spSHWinds.CreateInstance (__uuidof(SHDocVw::ShellWindows)); 
    
	if (FAILED (hr)) {
		return false;
	} else {

		_ASSERT (spSHWinds != NULL);
	    
		if(spSHWinds!=NULL) {
			try {
				long nCount = spSHWinds->GetCount ();
				
				IDispatchPtr spDisp;
				for (long i = 0; i < nCount; i++)
				{
					_variant_t va (i, VT_I4);
					spDisp = spSHWinds->Item (va);
			        
					IWebBrowser2 * pWebBrowser = NULL;
					hr = spDisp.QueryInterface (IID_IWebBrowser2, & pWebBrowser);
			        
					if (SUCCEEDED(hr) && pWebBrowser != NULL)
					{
						HWND hwndIE;
						DWORD dProcessId;
						pWebBrowser->get_HWND((SHANDLE_PTR*)&hwndIE);
						GetWindowThreadProcessId(hwndIE, &dProcessId);

						if(dProcessId==m_piProcessInfo.dwProcessId) {
							pInternetExplorer = pWebBrowser;
							spSHWinds.Release();
							CoUninitialize();
							return true;
						}

						pWebBrowser->Release ();
						pWebBrowser = NULL;
					}
				} 
			} catch(...) {
			}
			spSHWinds.Release();
		}
	}
	CoUninitialize(); 

    return false;
}

void 
InternetExplorerInstanceBulk::visitUrl(Url* url)
{
	hVisiting = CreateEvent(NULL, false, NULL, NULL);
	dwNetworkErrorCode = 0;
	major = CAPTURE_VISITATION_OK;
	mainURL.vt = VT_EMPTY;

	bool wait = true;
	HRESULT hr;
	DWORD iReturnVal = executeProcess(); //will set the browser object via the isOpen function

	if(iReturnVal==0) 
	{
		//connect and register event handler
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
			
		} else {
			printf("Error: Unable to find created IE window.\n");
			url->setMajorErrorCode(CAPTURE_VISITATION_ATTACH_PROCESS_ERROR);
		}
	
	} else {
		printf("Error: Unable to create IE window.\n");
		url->setMajorErrorCode(CAPTURE_VISITATION_PROCESS_ERROR);
	}

	CloseHandle(hVisiting);
}

bool
InternetExplorerInstanceBulk::Close()
{
	
	//first the process denoted by this process handler
	EnumWindows(InternetExplorerInstanceBulk::EnumWindowsCloseProc, (LPARAM) this);
	//lets try to terminate
	if(!TerminateProcess(m_piProcessInfo.hProcess, 0))
	{
		return false;
	}

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
	case 283:
		SetEvent(hVisiting);
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
	case DISPID_ONQUIT:
		exited = true;
		SetEvent(hVisiting);
		break;
	default:
		break;
	}
	return(DISP_E_UNKNOWNINTERFACE);
 }
