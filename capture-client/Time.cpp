/*
 *	PROJECT: Capture
 *	FILE: Time.cpp
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
#include "Precompiled.h"

std::wstring
Time::timefieldToString(const TIME_FIELDS& time)
{
	std::wstring stime;
	stime += boost::lexical_cast<std::wstring>(time.wDay);
	stime += L"/";
	stime += boost::lexical_cast<std::wstring>(time.wMonth);
	stime += L"/";
	stime += boost::lexical_cast<std::wstring>(time.wYear);
	stime += L" ";
	stime += boost::lexical_cast<std::wstring>(time.wHour);
	stime += L":";
	stime += boost::lexical_cast<std::wstring>(time.wMinute);
	stime += L":";
	stime += boost::lexical_cast<std::wstring>(time.wSecond);
	stime += L".";
	stime += boost::lexical_cast<std::wstring>(time.wMilliseconds);
	return stime;
}

std::wstring
Time::systemtimeToString(const SYSTEMTIME& time)
{
	std::wstring stime;
	stime += boost::lexical_cast<std::wstring>(time.wDay);
	stime += L"/";
	stime += boost::lexical_cast<std::wstring>(time.wMonth);
	stime += L"/";
	stime += boost::lexical_cast<std::wstring>(time.wYear);
	stime += L" ";
	stime += boost::lexical_cast<std::wstring>(time.wHour);
	stime += L":";
	stime += boost::lexical_cast<std::wstring>(time.wMinute);
	stime += L":";
	stime += boost::lexical_cast<std::wstring>(time.wSecond);
	stime += L".";
	stime += boost::lexical_cast<std::wstring>(time.wMilliseconds);
	return stime;
}

std::wstring
Time::getCurrentTime()
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	return systemtimeToString(st);
}