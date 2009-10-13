/*
 *	PROJECT: Capture
 *	FILE: VisitEvent.h
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
#include "Url.h"
#include "ErrorCodes.h"
#include <vector>


class VisitEvent
{
public:
	VisitEvent()
	{
		urlGroup = false;
		malicious = false;
		errorCode = CAPTURE_VISITATION_OK;
		algorithm = L"seq";
	}

	virtual ~VisitEvent()
	{
		for each(Url* url in urls)
		{
			delete url;
		}
		urlGroup = false;
	}

	void addUrl(Url* url)
	{
		urls.push_back(url);
		if(urls.size() > 1)
			urlGroup = true;
	}

	void setIdentifier(const std::wstring& id)
	{
		identifier = id;
	}

	void setProgram(const std::wstring& prog)
	{
		program = prog;
	}

	void setAlgorithm(const std::wstring& alg)
	{
		algorithm = alg;
	}
	
	void setMalicious(bool mal) { malicious = mal; }

	Element toElement() const
	{
		Element element;
		element.setName(L"visit-event");
		element.addAttribute(L"identifier", identifier);
		element.addAttribute(L"program", program);
		// TODO fix this. malicious attribute is added in the analyzer
		//element.addAttribute(L"malicious", boost::lexical_cast<std::wstring>(malicious));
		element.addAttribute(L"time", Time::getCurrentTime());
		element.addAttribute(L"algorithm",algorithm);
		for each(Url* url in urls)
		{
			Element* e = new Element(url->toElement());
			element.addChildElement(e);
		}
		return element;
	}

	void setErrorCode(unsigned long error)
	{
		if(errorCode != CAPTURE_VISITATION_OK)
		{
			errorCode = CAPTURE_VISITATION_MULTIPLE_ERRORS;
		} else {
			errorCode = error;
		}
	}

	inline const std::vector<Url*>& getUrls() const { return urls; }
	inline const std::wstring& getIndentifier() const { return identifier; }
	inline const std::wstring& getProgram() const { return program; }
	inline bool isUrlGroup() { return urlGroup; }
	inline bool isMalicious() { return malicious; }
	inline bool isError() { return (errorCode != CAPTURE_VISITATION_OK); }
	inline unsigned long getErrorCode() { return errorCode; }
private:
	std::vector<Url*> urls;
	std::wstring identifier;
	std::wstring program;
	std::wstring algorithm;

	bool urlGroup;
	bool malicious;
	unsigned long errorCode;
};
