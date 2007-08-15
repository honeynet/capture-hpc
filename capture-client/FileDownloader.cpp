#include "FileDownloader.h"

FileDownloader::FileDownloader(void)
{
	hDownloaded= CreateEvent(NULL, FALSE, FALSE, NULL);
	hConnected = CreateEvent(NULL, FALSE, FALSE, NULL);
	bDownloading = false;
}

FileDownloader::~FileDownloader(void)
{
	CloseHandle(hDownloaded);
	CloseHandle(hConnected);
}

bool
FileDownloader::Download(wstring what, wstring *to)
{
	DWORD dwStatus;
	BOOL folderCreated = FALSE;
	wstring tempPath = L"C:\\capture_temp";
	folderCreated = CreateDirectory(tempPath.c_str(), NULL);

	if(folderCreated == FALSE)
	{
		//printf("FileDownloader - ERROR - Could not create temp directory: %ls\n", tempPath.c_str());
		//return false;
	}
	tempPath += L"\\tempFile";
	
	wstring extension = what.substr(what.find_last_of(L"."));
	printf("Got extension: %ls\n", extension.c_str());
	tempPath += extension;

	DeleteFile(tempPath.c_str());

	printf("Downloading to: %ls\n", tempPath.c_str());
	*to = tempPath;

	URLDownloadToFile(NULL,
			what.c_str(),
			tempPath.c_str(),
			0,
			this
		);
	// Wait for connection signal
	dwStatus = WaitForSingleObject(hConnected, 30000);
	//ResetEvent(hDownloadStatus);

	if(dwStatus == WAIT_TIMEOUT)
	{
		printf("FileDownloader - ERROR - Connection timeout\n");
		return false;
	}
	printf("FileDownloader - Downloading file: %ls\n", what.c_str());
	// Wait for download to finish signal
	dwStatus = WaitForSingleObject(hDownloaded, 60000);
	if(dwStatus == WAIT_TIMEOUT)
	{
		printf("FileDownloader - ERROR - Download timeout\n");
		return false;
	}
	printf("FileDownloader - File Downloaded: %ls\n", what.c_str());
	return true;
}
