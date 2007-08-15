/*
 *	PROJECT: Capture
 *	FILE: Monitor.h
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
#include <PSAPI.h>
#include "Permission.h"

using namespace std;
/*
   Class: Monitor

   Provides a common interface for the construction of system monitors
*/

/*
   Constants: Kernel Driver IOCTL Codes

   IOCTL_CAPTURE_START - Starts the kernel drivers monitor.
   IOCTL_CAPTURE_STOP - Stops the kernel drivers monitor.
*/
#define IOCTL_CAPTURE_START    CTL_CODE(0x00000022, 0x0805, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_CAPTURE_STOP    CTL_CODE(0x00000022, 0x0806, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

typedef pair <wstring, std::list<Permission*>*> Permission_Pair;
typedef pair <DWORD, wstring> Process_Pair;

/*
	Struct: FILE_EVENT

	Contains the structure of the time passed from the kernel driver. Note this is
	undocumented API usage here.
*/
typedef struct _TIME_FIELDS {
	USHORT Year; 
	USHORT Month; 
	USHORT Day; 
	USHORT Hour; 
	USHORT Minute; 
	USHORT Second; 
	USHORT Milliseconds; 
	USHORT Weekday;
} TIME_FIELDS, *PTIME_FIELDS;

class Monitor
{	
public:
	virtual void Pause() = 0;
	virtual void UnPause() = 0;	
public:
	/*
		Function: TimeFieldToWString

		Converts a <TIME_FIELDS> structure to a readible wstring
	*/
	wstring TimeFieldToWString(TIME_FIELDS time);
	/*
		Function: EventIsAllowed
		Checks whether an event is allowed
	*/
	bool EventIsAllowed(std::wstring eventType, std::wstring subject, std::wstring object);
	
	/*
		Function: GetProcessCompletePathName
		Gets the complete path of the specified process id
	*/
	bool GetProcessCompletePathName(DWORD pid, std::wstring *path, bool created);
	
	/*
		Function: LogicalToDOSPath
		Converts a logical path to a dos path
	*/
	std::wstring LogicalToDOSPath(wchar_t *logicalPath);
	
	/*
		Function: LoadExclusionList
		Loads an exclusion list from a a file and creates a permission list
	*/
	bool LoadExclusionList(wstring file);
	
	/*
		Function: GetDriveLetter
		Parses a string and finds the actual driver it came from like c or d
	*/
	wchar_t GetDriveLetter(LPCTSTR lpDevicePath);
	
	/*
		Function: InstallKernelDriver
		Installs a kernel driver
	*/
	bool InstallKernelDriver(wstring driverFullPathName, wstring driverName, wstring driverDescription);
	
	/*
		Function: UnInstallKernelDriver
		Uninstalls a kernel driver
	*/
	void UnInstallKernelDriver();

	/*
         Variable:  hService
         A handle to an installed kernel driver
    */
	SC_HANDLE hService;

	/*
         Variable:  permissionMap
         A map containing a list of permissions based on a particular event type
    */
	stdext::hash_map<wstring, std::list<Permission*>*> permissionMap;

	/*
         Variable:  processNameMap
         Contains a mapping between process id's and the complete path names of that process
    */
	stdext::hash_map<DWORD, wstring> processNameMap;
};