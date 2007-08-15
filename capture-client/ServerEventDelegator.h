/*
 *	PROJECT: Capture
 *	FILE: ServerEventDelegator.h
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
#include <hash_map>
#include <list>
#include <vector>

using namespace stdext;
using namespace std;

typedef pair <wstring, std::list<ServerListener*>*> Event_Pair;

/*
	Class: ServerEventDelegator

	Parses messages from the server and dispatches them to objects that have requested to be notified
	of a particular event
*/
class ServerEventDelegator
{
public:
	ServerEventDelegator(void);

	/*
		Function: AddEventType

		Add an event type
	*/
	void AddEventType(wstring eventType);
	
	/* 
		Function: ReceiveEvent

		Receives a message from the <ServerReceive> class, splits it, and then passes the message onto all listening objects
	*/
	void ReceiveEvent(wchar_t* sEvent);

	/*
		Function: Register

		Register an object that implements the <ServerListener> interface and notify it if when a particular message type is received
	*/
	
	bool Register(ServerListener* caller, wstring eventType);
	/*
		Function: RemoveEventType

		Removes an event type
	*/
	void RemoveEventType(wstring eventType);

	/*
		Function: SplitString

		Splits a string containing the delimeter "::" and puts the tokens into a std::vector
	*/
	void SplitString(const wstring& str,vector<wstring>& tokens,const wstring& delimiters = L" ")
	{
		string::size_type lastPos = 0;
		string::size_type pos     = str.find(delimiters, lastPos);
		while (true)
		{
			if(pos == string::npos) {
				tokens.push_back(str.substr(lastPos));
				return;
			}
			tokens.push_back(str.substr(lastPos, pos - lastPos));
			lastPos = pos+delimiters.size();
			pos = str.find(delimiters, lastPos);
		}
	}

	/* 
		Function: UnRegister

		Unregisters a listening object so that it no longer will receive messages of a particular type
	*/
	bool UnRegister(ServerListener* caller, wstring eventType);

	~ServerEventDelegator(void);

	/* 
		Variable: eventTypes

		Hashmap that contains a mapping between message types and objects to be notified when that message is received
	*/
	stdext::hash_map<wstring, std::list<ServerListener*>*> eventTypeMap;
};