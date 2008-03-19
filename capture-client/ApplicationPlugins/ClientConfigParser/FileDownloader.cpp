/*
 *	PROJECT: Capture
 *	FILE: FileDownloader.cpp
 *	AUTHORS: Ramon Steenson (rsteenson@gmail.com) & Christian Seifert (christian.seifert@gmail.com)
 *
 *	Copyright Victoria University of Wellington 2008
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

DWORD
FileDownloader::Download(wstring what, wstring *toFile)
{
	DWORD dwStatus;
	BOOL folderCreated = FALSE;

	// Setup temp file paths etc 
	wstring tempFilePath = L"temp\\";
	//tempFilePath = *toFile;
	wchar_t* szTempPath = new wchar_t[1024];
	wchar_t* szTempFilePath = new wchar_t[1024];
	GetFullPathName(tempFilePath.c_str(), 1024, szTempFilePath, NULL);
	GetFullPathName(L"temp", 1024, szTempPath, NULL);
	tempFilePath = szTempFilePath;
	folderCreated = CreateDirectory(szTempPath, NULL);
	delete [] szTempPath;
	delete [] szTempFilePath;

	if(folderCreated == FALSE)
	{
		//printf("FileDownloader - ERROR - Could not create temp directory: %ls\n", tempPath.c_str());
		//return false;
	}
	tempFilePath += L"tempFile";
	
	wstring extension = what.substr(what.find_last_of(L"."));
	tempFilePath += extension;

	DeleteFile(tempFilePath.c_str());

	printf("Downloading to: %ls\n", tempFilePath.c_str());
	*toFile = tempFilePath;
	HRESULT result = URLDownloadToFile(NULL,
			what.c_str(),
			tempFilePath.c_str(),
			0,
			this
		);

	// Wait for connection signal
	dwStatus = WaitForSingleObject(hConnected, 30000);

	if(dwStatus == WAIT_TIMEOUT)
	{
		printf("FileDownloader - ERROR - Connection timeout\n");
		return CAPTURE_NE_CONNECT_ERROR_DL_TEMP_FILE;
	}
	printf("FileDownloader - Downloading file: %ls\n", what.c_str());

	// Wait for download to finish signal
	dwStatus = WaitForSingleObject(hDownloaded, 60000);
	if(dwStatus == WAIT_TIMEOUT)
	{
		printf("FileDownloader - ERROR - Download timeout\n");
		return CAPTURE_NE_CANT_DOWNLOAD_TEMP_FILE;
	}
	printf("FileDownloader - File Downloaded: %ls\n", what.c_str());
	return 0;
}
