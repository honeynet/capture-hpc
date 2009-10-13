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
#include <vector>
#include <hash_map>

class Permission;

#define CTL_CODE(devType, func, meth, acc) (((devType) << 16) | ((acc) << 14) | ((func) << 2) | (meth))
#define METHOD_BUFFERED				0
#define METHOD_IN_DIRECT			1
#define METHOD_OUT_DIRECT			2
#define METHOD_NEITHER				3
#define FILE_ANY_ACCESS				0
#define FILE_READ_ACCESS			( 0x0001 )
#define FILE_WRITE_ACCESS			( 0x0002 )
#define FILE_DEVICE_CONTROLLER		0x00000004

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

class Monitor
{
public:
	Monitor();
	virtual ~Monitor();

	virtual void start() = 0;
	virtual void stop() = 0;
		/*
		Function: clearExclusionList

		Clears all exclusions added through the exclusion lists. Excluded all the
		permaneant exclusions which are created during object creation.
	*/
	void clearExclusionList();

protected:
	/*
		Function: EventIsAllowed
		Checks whether an event is allowed
	*/
	bool isEventAllowed(const std::wstring& eventType, const std::wstring& subject, const std::wstring& object);
	/*
		Function: InstallKernelDriver
		Installs a kernel driver
	*/
	bool installKernelDriver(const std::wstring& driverPath, const std::wstring& driverName, const std::wstring& driverDescription);
	/*
		Function: UnInstallKernelDriver
		Uninstalls a kernel driver
	*/
	void unInstallKernelDriver();
	/*
		Function: LoadExclusionList
		Loads an exclusion list from a a file and creates a permission list
	*/
	void loadExclusionList(const std::wstring& file);
	/*
		Function: prepareStringForExclusion

		Helper function which parses a string for "." and adds a "\" in front of it
	*/
	void prepareStringForExclusion(std::wstring& s);

	/*
		Function: addExclusion

		Creates a permission and adds an the exclusion to the internal list
	*/
	void addExclusion(const std::wstring& excluded, const std::wstring& action, const std::wstring& subject, const std::wstring& object, bool permaneant = false);


	SC_HANDLE hService;

	/*
         Variable:  permissionMap
         A map containing a list of permissions based on a particular event type
    */
	stdext::hash_map<std::wstring, std::list<Permission*>*> permissionMap;
};
