/*
 *	PROJECT: Capture
 *	FILE: Permission.h
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
#include <PSAPI.h>

using namespace boost;

typedef enum _PERMISSION_CLASSIFICATION {
    NO_MATCH,
	ALLOWED,
	DISALLOWED
} PERMISSION_CLASSIFICATION;

/*
	Class: Permission
*/
class Permission
{
public:
	/*
		Function: Check
	*/
	PERMISSION_CLASSIFICATION Check(wstring subject, wstring object);

	/*
		Variable: subjects
	*/
	std::list<boost::wregex> subjects;

	/*
		Variable: permissions
	*/
	std::list<boost::wregex> permissions;

	/*
		Variable: allow
	*/
	bool allow;
};