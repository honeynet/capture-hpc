/*
 *	PROJECT: Capture
 *	FILE: ServerEventDelegator.cpp
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
#include "StdAfx.h"
#include "ServerEventDelegator.h"

ServerEventDelegator::ServerEventDelegator(void)
{
}

ServerEventDelegator::~ServerEventDelegator(void)
{
	stdext::hash_map<wstring, std::list<ServerListener*>*>::iterator it;
	
	for (it=eventTypeMap.begin(); it!= eventTypeMap.end(); it++)
	{
		std::list<ServerListener*>::iterator lit;
		delete it->second;
	}
}

void 
ServerEventDelegator::AddEventType(wstring eventType)
{
	stdext::hash_map<wstring, std::list<ServerListener*>*>::iterator it;

	it = eventTypeMap.find(eventType);
	
	if(it == eventTypeMap.end())
	{
		std::list<ServerListener*>*l = new list<ServerListener*>();
		eventTypeMap.insert(Event_Pair(eventType, l));
	}	
}

bool 
ServerEventDelegator::Register(ServerListener* caller, wstring eventType)
{
	stdext::hash_map<wstring, std::list<ServerListener*>*>::iterator it;

	it = eventTypeMap.find(eventType);
	
	if(it != eventTypeMap.end())
	{
		std::list<ServerListener*>* callbacks = it->second;
		callbacks->push_back(caller);
		return true;
	} else {
		return false;
	}
}

bool 
ServerEventDelegator::UnRegister(ServerListener* caller, wstring eventType)
{
	stdext::hash_map<wstring, std::list<ServerListener*>*>::iterator it;
	it = eventTypeMap.find(eventType);
	if(it != eventTypeMap.end())
	{
		std::list<ServerListener*>* callbacks = it->second;
		callbacks->remove(caller);
		return true;
	} else {
		return false;
	}
}

void 
ServerEventDelegator::RemoveEventType(wstring eventType)
{
	stdext::hash_map<wstring, std::list<ServerListener*>*>::iterator it;
	it = eventTypeMap.find(eventType);
	if(it != eventTypeMap.end())
	{
		std::list<ServerListener*>*l = new list<ServerListener*>();
		eventTypeMap.erase(eventType);
	}	
}

void 
ServerEventDelegator::ReceiveEvent(wchar_t* sEvent)
{	
	vector<wstring> v;
	wstring msg = sEvent;
	if(msg.size() < 2)
		return;
	msg = msg.erase(msg.size()-1);
	wstring del = L"::";	
	
	// Split the sEvent up using :: as delimeter
	SplitString(msg,v,del);

	if(v.size() > 1)
	{
		stdext::hash_map<wstring, std::list<ServerListener*>*>::iterator it;
		it = eventTypeMap.find(v[0]);
		if(it != eventTypeMap.end())
		{
			std::list<ServerListener*>* callbacks = it->second;
			std::list<ServerListener*>::iterator lit;

			// Inform all registered callbacks of event
			for (lit=callbacks->begin(); lit!= callbacks->end(); lit++)
			{
				(*lit)->OnReceiveEvent(v);
			}
		}
	}
}
