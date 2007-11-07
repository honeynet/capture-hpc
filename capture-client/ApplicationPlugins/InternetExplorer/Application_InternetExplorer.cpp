/*
 *	PROJECT: Capture
 *	FILE: Client_InternetExplorer.cpp
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
#include "Application_InternetExplorer.h"
#include "InternetExplorerInstance.h"

Application_InternetExplorer::Application_InternetExplorer()
{

}

Application_InternetExplorer::~Application_InternetExplorer(void)
{
}

void
Application_InternetExplorer::visitGroup(VisitEvent* visitEvent)
{	
	int i = 0;
	int numInstances = visitEvent->getUrls().size();
	InternetExplorerInstance* iexploreInstances = new InternetExplorerInstance[numInstances];
	HANDLE* hEvents = (HANDLE*)malloc(sizeof(HANDLE*)*numInstances);
	for each(Url* url in visitEvent->getUrls())
	{
		hEvents[i] = CreateEvent(NULL, false, NULL, NULL);
		iexploreInstances[i].visitUrl(url, hEvents[i]);
		i++;
	}

	// Wait for all the IE instances to report they are finished visiting or timeout
	DWORD dwWait = WaitForMultipleObjects(numInstances, hEvents, true, 60*1000);

	bool wait = true; // If no error occured then wait the visit time
	int errors = 0; // Count how many IE instances returned a bad error
	i = 0;
	for each(Url* url in visitEvent->getUrls())
	{
		if(dwWait == WAIT_TIMEOUT)
		{
			url->setMajorErrorCode(CAPTURE_VISITATION_TIMEOUT_ERROR);
			visitEvent->setErrorCode(CAPTURE_VISITATION_TIMEOUT_ERROR);
			wait = false;
		} else {
			visitEvent->setErrorCode(iexploreInstances[i].major);
			url->setMajorErrorCode(iexploreInstances[i].major);
			url->setMinorErrorCode(iexploreInstances[i].minor);

			if(iexploreInstances[i].minor >= 0x800C0000 && iexploreInstances[i].minor <= 0x800CFFFF) {
				wait = false;
				errors++;
			} else {
				url->setVisited(true);
			}
		}

		CloseHandle(hEvents[i++]);
	}

	if(wait || (visitEvent->getUrls().size() > 0 && visitEvent->getUrls().size() != errors) )
	{
		Sleep(visitEvent->getUrls()[0]->getVisitTime()*1000);

	}

	delete [] iexploreInstances;
	free(hEvents);
}

wchar_t**
Application_InternetExplorer::getSupportedApplicationNames()
{
	return supportedApplications;
}