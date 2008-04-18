/*
 *	PROJECT: Capture
 *	FILE: ProcessHandler.h
 *	AUTHORS: Ramon Steenson (rsteenson@gmail.com) & Christian Seifert (christian.seifert@gmail.com)
 *
 *	Copyright Victoria University of Wellington 2008
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

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <iostream>
#include <strsafe.h>


class ProcessHandler
{
public:
	ProcessHandler(std::wstring fullPathToExe, std::wstring parameters);
	~ProcessHandler(void);
	DWORD executeProcess();
	DWORD closeProcess(void);
	DWORD getProcessId(void);
	bool isOpen(void);

private:
	std::wstring m_fullPathToExe;
	std::wstring m_parameters;
	PROCESS_INFORMATION m_piProcessInfo;
	BOOL static CALLBACK EnumWindowsCloseProc(HWND hwnd,LPARAM lParam);
	BOOL static CALLBACK EnumWindowsCloseAppProc(HWND hwnd,LPARAM lParam);
	int compareName(DWORD processID, std::wstring processName);
	void DebugPrintTrace(LPCTSTR pszFormat, ... );


};
