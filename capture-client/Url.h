/*
 *	PROJECT: Capture
 *	FILE: Url.h
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
#include "Element.h"
#include "CaptureGlobal.h"
#include <string>

/*
	Class: Url
	
	Object which holds the url information such as the actual URL and the visit time
*/
class Url
{
public:
	Url(std::wstring u, std::wstring app, int vTime);
	virtual ~Url(void);

	void setUrl(const std::wstring& u) { url = u; }
	void setProgram(const std::wstring & prog) { program = prog; }
	void setVisitTime(int time) { visitTime = time; }
	void setVisited(bool vis) { visited = vis; }
	void setMajorErrorCode(unsigned long error) { majorErrorCode = error; }
	void setMinorErrorCode(unsigned long error) { minorErrorCode = error; }

	Element toElement();

	inline const std::wstring getUrl() const { return url; }
	inline const std::wstring getProgram() const { return program; }
	inline int getVisitTime() { return visitTime; }
	inline bool isVisited() { return visited; }

private:
	std::wstring url;
	std::wstring program;
	int visitTime;

	bool visited;
	bool malicious;
	unsigned long majorErrorCode;
	unsigned long minorErrorCode;

public:
	static std::wstring encode(const std::wstring& url);
	static std::wstring decode(const std::wstring& url);
private:
	static const wchar_t * hexenc[];
};
