/*
 *	PROJECT: Capture
 *	FILE: Application_ClientConfigManager.h
 *	AUTHORS: Ramon Steenson (rsteenson@gmail.com) & Christian Seifert (christian.seifert@gmail.com)
 *
 *	Developed by the New Zealand Honeynet Project
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

#include "..\..\ApplicationPlugin.h"

using namespace std;

/* Define the application plugin name and the application names it supports */
#define APPLICATION_PLUGIN_NAME		Application_Safari
static wchar_t* supportedApplications[] = {L"safari", NULL};

static const wchar_t* safariBlankPreferences = 
	L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
	L"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n" \
	L"<plist version=\"1.0\">\n" \
		L"<dict>\n" \
		L"</dict>\n" \
	L"</plist>\n";

class Application_Safari : public ApplicationPlugin
{
public:
	Application_Safari(void);
	~Application_Safari(void);
	
	void visitGroup(VisitEvent* visitEvent);
	DWORD visitUrl(Url* url, PROCESS_INFORMATION* piProcessInfo);
	wchar_t** getSupportedApplicationNames()
	{	
		return supportedApplications;
	}
	unsigned int getPriority() { return 100; };
	std::wstring getAlgorithm() { return L"seq"; }
private:
	bool openDocument(const wstring& file, std::wstring& document);
	void writeDocument(const wstring& file, const wstring& document);

};

/* DO NOT REMOVE OR MOVE THIS LINE ... ADDS EXPORTS (NEW/DELETE) TO THE PLUGIN */
/* Must be included after the actual plugin class definition */
#include "..\..\PluginExports.h"
