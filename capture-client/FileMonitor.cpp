/*
 *	PROJECT: Capture
 *	FILE: FileMonitor.cpp
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
#include "FileMonitor.h"

FileMonitor::FileMonitor()
{
	HRESULT hResult;
	installed = false;
	int tryTurn = 0;
	filemonThread = NULL;
	commPort = INVALID_HANDLE_VALUE;

	hResult = FilterLoad(L"CaptureFileMonitor");
	while((tryTurn < 5) && IS_ERROR( hResult ))
	{
		tryTurn++;
		printf("FileMonitor: WARNING - Filter driver not loaded (error: %x) waiting 3 seconds to try again ... (try %i of 5)\n", hResult, tryTurn);
		Sleep(3000);
		hResult = FilterLoad(L"CaptureFileMonitor");
		if (!IS_ERROR( hResult )) {
			printf("FileMonitor: Filter successfully loaded\n");
		}
	}

	if (!IS_ERROR( hResult )) {

		hResult = FilterConnectCommunicationPort( CAPTURE_FILEMON_PORT_NAME,
												0,
												 NULL,
												 0,
												NULL,
												&commPort );
		if (IS_ERROR( hResult )) {

			printf( "FileMonitor: ERROR - Could not connect to filter: 0x%08x\n", hResult );
		} else {
			installed = true;
		}
		wchar_t exListDriverPath[1024];
		GetCurrentDirectory(1024, exListDriverPath);
		wcscat_s(exListDriverPath, 1024, L"\\FileMonitor.exl");
		Monitor::LoadExclusionList(exListDriverPath);

		UCHAR buffer[1024];
		PFILTER_VOLUME_BASIC_INFORMATION volumeBuffer = (PFILTER_VOLUME_BASIC_INFORMATION)buffer;
		HANDLE volumeIterator = INVALID_HANDLE_VALUE;
		ULONG volumeBytesReturned;
		WCHAR driveLetter[15];

		hResult = FilterVolumeFindFirst( FilterVolumeBasicInformation,
											 volumeBuffer,
											 sizeof(buffer)-sizeof(WCHAR),   //save space to null terminate name
											 &volumeBytesReturned,
											 &volumeIterator );
		if (volumeIterator != INVALID_HANDLE_VALUE) {
			do {
				assert((FIELD_OFFSET(FILTER_VOLUME_BASIC_INFORMATION,FilterVolumeName) + volumeBuffer->FilterVolumeNameLength) <= (sizeof(buffer)-sizeof(WCHAR)));
				__analysis_assume((FIELD_OFFSET(FILTER_VOLUME_BASIC_INFORMATION,FilterVolumeName) + volumeBuffer->FilterVolumeNameLength) <= (sizeof(buffer)-sizeof(WCHAR)));
				volumeBuffer->FilterVolumeName[volumeBuffer->FilterVolumeNameLength/sizeof( WCHAR )] = UNICODE_NULL;
					
				if(SUCCEEDED( FilterGetDosName(volumeBuffer->FilterVolumeName,
					driveLetter,
					sizeof(driveLetter)/sizeof(WCHAR) )
					))
				{
					wstring dLetter = driveLetter;
					dosNameMap.insert(Dos_Pair(volumeBuffer->FilterVolumeName, dLetter));
				}
			} while (SUCCEEDED( hResult = FilterVolumeFindNext( volumeIterator,
																FilterVolumeBasicInformation,
																volumeBuffer,
																sizeof(buffer)-sizeof(WCHAR),    //save space to null terminate name
																&volumeBytesReturned ) ));
		}

		if (INVALID_HANDLE_VALUE != volumeIterator) {
			FilterVolumeFindClose( volumeIterator );
		}
	}
}

FileMonitor::~FileMonitor(void)
{
	FilterUnload(L"CaptureFileMonitor");
	if(filemonThread != NULL)
		filemonThread->Stop();
	if(commPort != INVALID_HANDLE_VALUE)
		CloseHandle(commPort);
	
}

void 
FileMonitor::Start()
{
	if(!installed)
	{
		printf("FileMonitor: ERROR - filter driver was not loaded properly\n");
		return;
	}
	filemonThread = new Thread(this);
	filemonThread->Start();
}

void 
FileMonitor::Stop()
{
	filemonThread->Stop();
}

void 
FileMonitor::Pause()
{
	FILEMONITOR_MESSAGE command;
	command.Command = FStop;
	HRESULT hResult;
	DWORD bytesReturned = 0;
	hResult = FilterSendMessage( commPort,
                                 &command,
                                 sizeof( FILEMONITOR_COMMAND ),
                                 0,
                                 0,
                                 &bytesReturned );
}

void 
FileMonitor::UnPause()
{
	FILEMONITOR_MESSAGE command;
	command.Command = FStart;
	HRESULT hResult;
	DWORD bytesReturned = 0;

	hResult = FilterSendMessage( commPort,
                                 &command,
                                 sizeof( FILEMONITOR_COMMAND ),
                                 0,
                                 0,
                                 &bytesReturned );
}

void 
FileMonitor::Run()
{
	HRESULT hResult;
	FILE_EVENT fEvent[250];
	DWORD bytesReturned = 0;
	wchar_t szTemp[256];
	while(true)
	{
		FILEMONITOR_MESSAGE command;
		command.Command = GetFileEvents;

		ZeroMemory(&fEvent, sizeof(FILE_EVENT)*250);

		hResult = FilterSendMessage( commPort,
                                     &command,
                                     sizeof( FILEMONITOR_COMMAND ),
                                     fEvent,
                                     sizeof(FILE_EVENT)*250,
                                     &bytesReturned );
		for(unsigned int i = 0; i < (bytesReturned/sizeof(FILE_EVENT)); i++)
		{
			wstring eventName;
			switch(fEvent[i].type)
			{
			case FilePreRead:
				eventName = L"Read";
				break;
			case FilePreWrite:
				eventName = L"Write";
				break;
			default:
				break;
			}
			
			ZeroMemory(&szTemp, sizeof(szTemp));
			wstring processModuleName;
			wstring processPath;
			// Get process name and path
			if(Monitor::GetProcessCompletePathName(fEvent[i].processID, &processPath, true))
			{
				processModuleName = processPath.substr(processPath.find_last_of(L"\\")+1);
			}
			
			//WCHAR driveLetter[15];
			//wstring filePath = fEvent[i].name;
			//FilterGetDosName(filePath.c_str(),driveLetter,sizeof(driveLetter)/sizeof(WCHAR)); 

			wstring filePath = fEvent[i].name;
			stdext::hash_map<wstring, wstring>::iterator it;
			for(it = dosNameMap.begin(); it != dosNameMap.end(); it++)
			{
				size_t position = filePath.rfind(it->first,0);
				if(position != wstring::npos)
				{
					filePath.replace(position, it->first.length(), it->second, 0, it->second.length());
				}
			}
			//printf("Got: %ls\n", filePath.c_str());
		
			//wstring procModuleName = szTemp;
			//CloseHandle(hProc);
			if(!Monitor::EventIsAllowed(eventName,processPath,filePath))
			{
				//wstring eventPath = fEvent[i].name;
				
				wstring time = Monitor::TimeFieldToWString(fEvent[i].time);

				NotifyListeners(eventName, time, processPath, filePath);
			}
		}
		Sleep(500);
	}
}

void
FileMonitor::NotifyListeners(wstring eventType, wstring time, wstring processFullPath, wstring eventPath)
{
	std::list<FileListener*>::iterator lit;

	// Inform all registered listeners of event
	for (lit=fileListeners.begin(); lit!= fileListeners.end(); lit++)
	{
		(*lit)->OnFileEvent(eventType, time, processFullPath, eventPath);
	}
}

void 
FileMonitor::AddFileListener(FileListener* pl)
{
	fileListeners.push_back(pl);
}

void
FileMonitor::RemoveFileListener(FileListener* pl)
{
	fileListeners.remove(pl);
}