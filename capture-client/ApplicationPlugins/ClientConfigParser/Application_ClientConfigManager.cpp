/*
 *	PROJECT: Capture
 *	FILE: Application_ClientConfigManager.cpp
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
#include "Application_ClientConfigManager.h"

Application_ClientConfigManager::Application_ClientConfigManager()
{
	loadApplicationsList();
}

Application_ClientConfigManager::~Application_ClientConfigManager(void)
{
	for(int i = 0; supportedApplications[i] != NULL; i++)
	{
		delete supportedApplications[i];
	}
	delete [] supportedApplications;
}

void
Application_ClientConfigManager::visitGroup(VisitEvent* visitEvent)
{
	DWORD status = CAPTURE_VISITATION_OK;
	stdext::hash_map<wstring, APPLICATION*>::iterator it;
	it = applicationsMap.find(visitEvent->getProgram());
	if(it != applicationsMap.end())
	{	
		const size_t size = visitEvent->getUrls().size();
		std::vector<ProcessHandler*> processHandlers(size);
		
		bool error = false;
		for(unsigned int i = 0; i < visitEvent->getUrls().size(); i++)
		{	
			// Download file to temp directory if required
			Url* url = visitEvent->getUrls()[i];
			wstring url_path = url->getUrl();
			if(it->second->downloadURL)
			{
				FileDownloader* downloader = new FileDownloader();
				wstring file = url->getUrl().substr(url->getUrl().find_last_of(L"/"));
				DWORD downloadStatus = 0;
				if((downloadStatus = downloader->Download(url->getUrl(), &file)) > 0)
				{
					visitEvent->setErrorCode(CAPTURE_VISITATION_NETWORK_ERROR);
					url->setMajorErrorCode(CAPTURE_VISITATION_NETWORK_ERROR);
					url->setMinorErrorCode(downloadStatus);
					continue;
				} else {
					url_path = file;
				}
			}

			PAPPLICATION pApplication = it->second;
			wstring cmd = pApplication->path;
			wstring param = url_path;

			ProcessHandler *ph = new ProcessHandler(cmd,param);
			processHandlers[i] = ph;

			ph->executeProcess();
			url->setProcessId(ph->getProcessId());

			if(i==0) 
			{
				Sleep(2000);
			}

			double maxWaitTimeInSec = 60;
			double waitTimeInSec = 0;

			bool isOpen = ph->isOpen();
			while(!isOpen && waitTimeInSec < maxWaitTimeInSec) 
			{
				Sleep(100);
				waitTimeInSec = waitTimeInSec + 0.1;
				isOpen = ph->isOpen();
			}
			if(waitTimeInSec >= maxWaitTimeInSec)
			{
				visitEvent->setErrorCode(CAPTURE_VISITATION_PROCESS_ERROR);
				url->setMajorErrorCode(CAPTURE_VISITATION_PROCESS_ERROR);
				url->setMinorErrorCode(GetLastError());
				error = true;
				printf("Error opening app");
			} else {
				;//printf("Successfully opened app");
			}
			
		}
				
		if(!error)
		{
			int sleepTimeInSec = visitEvent->getUrls()[0]->getVisitTime();
			Sleep(sleepTimeInSec*1000);
		}

		for(unsigned int i = 0; i < visitEvent->getUrls().size(); i++)
		{
			DWORD minorError = 0;
			Url* url = visitEvent->getUrls()[i];
			ProcessHandler *ph = processHandlers[i];
			ph->closeProcess();

			double maxWaitTimeInSec = 30;	
			double waitTimeInSec = 0;
			bool isOpen = ph->isOpen();
			while(isOpen && waitTimeInSec < maxWaitTimeInSec) 
			{
				Sleep(100);
				waitTimeInSec = waitTimeInSec + 0.1;
				isOpen = ph->isOpen();
			}

			if(waitTimeInSec >= maxWaitTimeInSec)
			{
				visitEvent->setErrorCode(CAPTURE_VISITATION_PROCESS_ERROR);
				url->setMajorErrorCode(CAPTURE_VISITATION_PROCESS_ERROR);
				url->setMinorErrorCode(minorError);
			} else {
				;//printf("Successfully closed app");
			}
		}
		//delete [] processHandlers;
	} else {
		visitEvent->setErrorCode(CAPTURE_VISITATION_PROCESS_ERROR);
		//url->setMajorErrorCode(CAPTURE_VISITATION_PROCESS_ERROR);
		//url->setMinorErrorCode(CAPTURE_PE_PROCESS_PATH_NOT_FOUND);
	}
}


void 
Application_ClientConfigManager::loadApplicationsList()
{
	wstring line;
	int inserted = 0;
	wchar_t szFullPath[4096];
	GetFullPathName(L"Applications.conf", 4096, szFullPath, NULL);
	wifstream configF (szFullPath);
	if (!configF.is_open())
	{
		printf("Application_ConfigManager: Could not open the application config file: Applications.conf\n");
		supportedApplications = new wchar_t*[1];
		supportedApplications[0] = NULL;
		return;
	}
	while (!configF.eof())
	{
		getline (configF,line);
		if((line.size() > 0) && (line.at(0) != '#'))
		{
			vector<std::wstring> splitLine;			
			for(sf_it it=make_split_iterator(line, token_finder(is_any_of(L"\t")));
				it!=sf_it(); ++it)
			{
				splitLine.push_back(copy_range<std::wstring>(*it));				
			}

			if(splitLine.size() >= 2)
			{
				bool downloadURL = false;
				if(splitLine.size() == 3)
				{
					if(splitLine[2] == L"yes")
						downloadURL = true;
				}
				if(applicationsMap.find(splitLine[0]) == applicationsMap.end())
				{
					APPLICATION* a = new APPLICATION();
					a->name = splitLine[0];
					a->path = splitLine[1];
					a->downloadURL = downloadURL;
					applicationsMap.insert(ApplicationPair(splitLine[0], a));
					inserted++;
				}
			}
		}

	}
	supportedApplications = new wchar_t*[inserted+1];
	int i = 0;
	stdext::hash_map<wstring, PAPPLICATION>::iterator vit;
	for(vit = applicationsMap.begin(); vit != applicationsMap.end(); vit++)
	{
		supportedApplications[i] = new wchar_t[vit->first.length()+1];
		wcscpy_s(supportedApplications[i],vit->first.length()+1, vit->first.c_str());
		i++;
	}
	supportedApplications[i] = NULL;
	configF.close();
}

