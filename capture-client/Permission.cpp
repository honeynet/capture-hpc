/*
 *	PROJECT: Capture
 *	FILE: Permission.cpp
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
#include "Permission.h"

PERMISSION_CLASSIFICATION
Permission::Check(std::wstring subject, std::wstring object)
{
	std::list<boost::wregex>::iterator lit;
	boost::wcmatch what;
	bool found = false;

	for(lit = subjects.begin(); lit != subjects.end(); lit++)
	{
		if(boost::regex_match(subject.c_str(), what, (*lit)))
		{
			found = true;
		}
	}
	if(!found)
	{
		return NO_MATCH;
	}

	for(lit = permissions.begin(); lit != permissions.end(); lit++)
	{
		if(boost::regex_match(object.c_str(), what, (*lit)))
		{
			if(allow)
			{
				return ALLOWED;
			} else {
				return DISALLOWED;
			}			
		}
	}
	return NO_MATCH;
}
