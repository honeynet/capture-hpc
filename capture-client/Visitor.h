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
#include "ServerListener.h"
#include <queue>
#include <string>
#include <vector>
#include "Thread.h"
#include <list>
#include <list>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <boost\regex.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp> 
#include <boost/tokenizer.hpp>
#include <urlmon.h>
#include "VisitorListener.h"
#include "FileDownloader.h"
#include "Client.h"
#include "windows.h"


using namespace std;
using namespace boost;

typedef pair <wstring, Client*> Client_Pair;
typedef pair <wstring, int> Url_Pair;
typedef pair <Client_Pair, Url_Pair*> Visit_Pair;

typedef split_iterator<string::iterator> sf_it;

/*
	Class: Visitor

	Listens for visit events from the <ServerEventDelegator>. When one is received, it finds the url requested to visit
	and which client to use, and then proceeds to visit the url by creating a the process requested and instructing it
	to visit the url.
*/

class Visitor : public ServerListener, IRunnable
{
public:
	static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
	Visitor();
	Visitor(Common* g);
	~Visitor(void);

	/*
		Function: LoadClientsList

		Loads a list of clients from a file and puts it into the hashmap <visitorClientsMap>
	*/
	bool LoadClientsList(wstring path);

	/*
		Function: LoadClientPlugins

		Loads all the dll files labelled client_* and creates a IClient interface for each
		which the visitor compononent can call to pass the visitation process to the dll
		This overwrites the default behavious defined in the clients.conf if a plugin
		uses an already defined client in clients.conf
	*/
	void LoadClientPlugins(wstring directory);

	/*
		Function: OnReceiveEvent

		When a visit event is received from the server the <ServerEventDelegator> will call this function. This
		will put the message into the <toVisit> queue to be later visited by a particular client
	*/
	void OnReceiveEvent(vector<wstring> e);

	/*
		Function: AddVisitorListener

		Adds an object that implements the <VisitorListener> interface to be notified when the <Visitor> component
		starts and stops visiting a url
	*/
	void AddVisitorListener(VisitorListener* vl);

	/*
		Function: RemoveVisitorListener

		Remove a listening object so that it no longer gets notified when the <Visitor> starts and stops visiting a url
	*/
	void RemoveVisitorListener(VisitorListener* vl);

	/*
		Function: NotifyListeners

		Notify listeners of when the <Visitor> has started visiting, stopped visiting, or an error has occured during visitation
	*/
	void NotifyListeners(int eventType, DWORD minorEventCode, wstring url, wstring visitor);

	/*
		Function: Run

		A runnable thread that waits for urls to be queued in <toVisit> and then finds find the required client to visit
		and creates the process and instructs it to visit it.
	*/
	void Run();

	/*
		Function: GetVisitorClients

		Returns the <visitorClientsMap> hash map
	*/
	stdext::hash_map<wstring, Client*>* GetVisitorClients();

protected:
	Common* global;

	/*
		Variable: hQueueNotEmpty

		Handle which the <Run> thread waits on until it is signalled. It is signalled when a visit event is received
		in <OnReceiveEvent>
	*/
	HANDLE hQueueNotEmpty;
	
	/*
		Variable: toVisit

		Queue containing the url to visit and which client to use
	*/
	queue<Visit_Pair*> toVisit;

	/*
		Variable: visitListeners

		List of listening objects that implement the <VisitorListener> interface
	*/
	std::list<VisitorListener*> visitListeners;

	/*
		Variable: visitorClientsMap

		Mapping of clients executable names to the complete path name where it is found
	*/
	stdext::hash_map<wstring, Client*> visitorClientsMap;

	/*
		Variable: visiting

		Whether or not the visitor component is busy visiting a url
	*/
	bool visiting;

	/*
		Variable: clientPlugins

		Stores a linked list of loaded plugins
	*/
	std::list<HMODULE> clientPlugins;
};
