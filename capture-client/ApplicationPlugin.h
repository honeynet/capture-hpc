/*
 *	PROJECT: Capture
 *	FILE: ApplicationPlugin.h
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
#include <windows.h>
#include <string>
#include <list>
#include "VisitEvent.h"
#include "ErrorCodes.h"

using namespace std;

class ApplicationPlugin
{
public:
	/* Visits a particular url. After visitation, this function needs to
	   return one of the status codes defined in ErrorCodes.h. Optionally
	   a minor error code can be placed into the minorError parameter. */
	virtual void visitGroup(VisitEvent* visitEvent) = 0;
	/* Returns a pointer to an array that is allocated in the plugin
	   and contains the list of supported application names */
	virtual wchar_t** getSupportedApplicationNames() = 0;
	/* Returns the priority of the plugin 0 .. 2^32. 0 being the lowest.
	   The priority is used so Capture can determine what plugin to use
	   if there are two which support the same application name */
	virtual unsigned int getPriority() = 0;
	/* Returns which algorithm is supported by this plugin. This is only 
	   relevant when the group size is larger than 1. We currently support:
	   bulk - here the state changes and the visit finish/error event contains
	          process ID which allows the server to map a specific state change
			  to a URL. A client can support Bulk if:
			   - it can be started with multiple instances of which each instance 
			     visits a URL
			   - it can be started in its own process for each URL
			   - it can be started in a way that allows one to derive its processID
			   - processID of each URL needs to be set on the URL object
	   dac -  here the urls are visited in bulk and if a unauthorized state change is
	          detected, a divide-and-conquer algorithm is applied to find the specific
			  malicious URL (note that since URLs are visited multiple times, there is
			  a danger of loosing malicious URLs due to IP tracking functionality. A 
			  caching mechanism needs to be applied). A client can support DAC if:
			   - it can be started with multiple instances of which each instance 
			     visits a URL
	   seq -  here the urls are visited one after another. This is the slowest method, but 
	          doesnt require special capabilities by the plugin.

	*/
	virtual std::wstring getAlgorithm() = 0;
};
