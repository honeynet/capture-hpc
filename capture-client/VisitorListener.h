/*
 *	PROJECT: Capture
 *	FILE: VisitorListener.h
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
#pragma once
#include <string>
#include <vector>
#include <windows.h>

using namespace std;

/*
	Interface: VisitorListener

	Provides an interface for objects to implement so that they can be notified when the <Visitor> start and stops visiting
	a url
*/
class VisitorListener {
public:
	/*
		Function: OnVisitEvent

		Gets called when the <Visitor> component visits a url. Passes different eventType's (<VisitorEventTypes>)
	*/
	virtual void OnVisitEvent(int eventType, DWORD minorEventCode, wstring url, wstring visitor) = 0;
};

/*
	Enum: VisitorEventTypes

	Visitor event types that are passed along with a call to <OnVisitEvent> that are in objects which implement the <VisitorListener>
	interface

	VISIT_START - When the process is about to be opened and the url visited
	VISIT_FINISH - When the process has been terminated and the url visited
	VISIT_ERROR - When an error occurs when the url is being visited
	VISIT_PRESTART - Signalled before anything is done to visit the url
	VISIT_POSTFINISH - Signalled after a complete visit
*/
