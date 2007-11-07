/*
 *	PROJECT: Capture
 *	FILE: OptionsManager.h
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
#include "Element.h"
#include <string>
#include <boost/signal.hpp>
#include <hash_map>

using namespace std;
/*
	Class: OptionsManager

	The OptionsManager is responsible for managing the global options available in the
	client. When the client is run the command arguments are parsed and sent to this
	object so that they are globally available to all other objects in the system. It
	is a singleton so only one object can exist and provides an easy centralised place
	to manage options. In the future when the standalone version is worked on more this
	will allow a config file to be saved to a disk easily so that options can be persistant
	and changed at any time
*/
class OptionsManager
{
public:
	typedef boost::signal<void (std::wstring)> signal_optionChanged;
public:
	~OptionsManager(void);
	static OptionsManager* getInstance();

	const std::wstring getOption(const std::wstring& option);
	bool addOption(const std::wstring& option, const std::wstring& value);

	boost::signals::connection connect_onOptionChanged(const signal_optionChanged::slot_type& s);

private:
	void onOptionEvent(const Element& element);
	OptionsManager(void);
	static bool instanceCreated;
	static OptionsManager* optionsManager;
	signal_optionChanged signalOnOptionChanged;

	

	stdext::hash_map<std::wstring, std::wstring> optionsMap;
	boost::signals::connection onOptionEventConnection;
};
