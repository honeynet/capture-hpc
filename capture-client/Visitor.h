/*
 *	PROJECT: Capture
 *	FILE: Visitor.h
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
#include "CaptureGlobal.h"
#include "Thread.h"
#include "Element.h"
#include "Observable.h"
#include <string>
#include <queue>
#include <vector>
#include <hash_map>
#include <boost/signal.hpp>

class Url;
class ApplicationPlugin;
class VisitEvent;


/*
	Class: Visitor

	Listens for visit events from the <EventController>. When one is received, 
	it finds the url requested to visit and which client to use, and then proceeds 
	to visit the url by creating the process requested and instructing it to visit 
	the url.
*/
class Visitor : public Runnable, public Observable<VisitEvent>
{
public:
	typedef std::pair <HMODULE, std::list<ApplicationPlugin*>*> PluginPair;
	typedef std::pair <std::wstring, ApplicationPlugin*> ApplicationPair;
public:
	Visitor(void);
	virtual ~Visitor(void);

private:
	void loadClientPlugins();
	void unloadClientPlugins();
	Url* createUrl(const std::vector<Attribute>& attributes);
	ApplicationPlugin* getApplicationPlugin(const std::wstring& applicationName);

	void run();

	void onServerEvent(const Element& element);

	ApplicationPlugin* createApplicationPluginObject(HMODULE hPlugin);

	boost::signals::connection onServerEventConnection;

	HANDLE hQueueNotEmpty;
	bool visiting;
	std::queue<VisitEvent*> toVisit;
	Thread* visitorThread;
	stdext::hash_map<HMODULE, std::list<ApplicationPlugin*>*> applicationPlugins;
	stdext::hash_map<std::wstring, ApplicationPlugin*> applicationMap;
};
