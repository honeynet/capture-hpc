#pragma once
#include <Urlmon.h>
#include <string>

using namespace std;

class FileDownloader : public IBindStatusCallback
{
public:
	FileDownloader(void);
	~FileDownloader(void);

	bool Download(wstring what, wstring *to);

    HRESULT STDMETHODCALLTYPE OnProgress( 
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax,
            /* [in] */ ULONG ulStatusCode,
            /* [in] */ LPCWSTR szStatusText)
	{
		if(ulStatusCode == BINDSTATUS_DOWNLOADINGDATA)
		{
			if(!bDownloading)
			{
				bDownloading = true;
				SetEvent(hConnected);
			}
		} else if(ulStatusCode == BINDSTATUS_ENDDOWNLOADDATA)
		{
			if(!bDownloading)
			{
				bDownloading = true;
				SetEvent(hConnected);
			}
			SetEvent(hDownloaded);
		}
		return(NOERROR);
	}

	HRESULT STDMETHODCALLTYPE OnStartBinding( 
            /* [in] */ DWORD dwReserved,
            /* [in] */ IBinding *pib)
	{
		return(E_NOTIMPL);
	}
        
    HRESULT STDMETHODCALLTYPE GetPriority( 
            /* [out] */ LONG *pnPriority)
	{
		return(E_NOTIMPL);
	}
        
    HRESULT STDMETHODCALLTYPE OnLowResource( 
            /* [in] */ DWORD reserved)
	{
		return(E_NOTIMPL);
	}
        
    HRESULT STDMETHODCALLTYPE OnStopBinding( 
            /* [in] */ HRESULT hresult,
            /* [unique][in] */ LPCWSTR szError)
	{
		return(E_NOTIMPL);
	}
        
    HRESULT STDMETHODCALLTYPE GetBindInfo( 
            /* [out] */ DWORD *grfBINDF,
            /* [unique][out][in] */ BINDINFO *pbindinfo)
	{
		return(E_NOTIMPL);
	}
        
    HRESULT STDMETHODCALLTYPE OnDataAvailable( 
            /* [in] */ DWORD grfBSCF,
            /* [in] */ DWORD dwSize,
            /* [in] */ FORMATETC *pformatetc,
            /* [in] */ STGMEDIUM *pstgmed)
	{
		return(E_NOTIMPL);
	}
        
    HRESULT STDMETHODCALLTYPE OnObjectAvailable( 
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown *punk)
	{
		return(E_NOTIMPL);
	}

	HRESULT STDMETHODCALLTYPE QueryInterface( 
                /* [in] */ REFIID riid,
                /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		return(E_NOTIMPL);
	}
            
	ULONG STDMETHODCALLTYPE AddRef( void)
	{
		return(E_NOTIMPL);
	}
            
	ULONG STDMETHODCALLTYPE Release( void)
	{
		return(E_NOTIMPL);
	}
private:
	HANDLE hDownloaded;
	HANDLE hConnected;
	bool bDownloading;
};
