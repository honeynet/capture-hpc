/*
 *	PROJECT: Capture
 *	FILE: ProcessMonitor.h
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
#include <windows.h>
#include <winioctl.h>
#include "Thread.h"
#include "Visitor.h"
#include "VisitorListener.h"
#include "Monitor.h"
#include "RegistryMonitor.h"
#include <PSAPI.h>

/*
   Constants: Kernel Driver IOCTL Codes

   IOCTL_CAPTURE_GET_PROCINFO - Passes a userspace buffer to the process monitor kernel driver and returns with that buffer containing process events
*/
#define IOCTL_CAPTURE_GET_PROCINFO	CTL_CODE(0x00000022, 0x0802, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_CAPTURE_PROC_LIST    CTL_CODE(0x00000022, 0x0807, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)
/*
	Struct: ProcessInfo

	Process structure from kernel driver
*/
typedef struct _ProcessInfo
{
	TIME_FIELDS time;
	DWORD ParentId;
	WCHAR parentProcPath[1024];
	DWORD ProcessId;
	WCHAR procPath[1024];
	BOOLEAN bCreate;
} ProcessInfo;

typedef struct _PROCESS_TUPLE
{
	DWORD processID;
	WCHAR name[1024];
} PROCESS_TUPLE, * PPROCESS_TUPLE;

/*
	Class: ProcessListener

	Interface for object to implement so that they can be notified of process events
*/
class ProcessListener
{
public:
	virtual void OnProcessEvent(BOOLEAN bCreate, wstring time, wstring processParentPath, wstring processPath) = 0;
};

/*
	Class: ProcessMonitor

	Implements: <IRunnable>, <VisitorListener>, <Monitor>
*/
class ProcessMonitor : public IRunnable, VisitorListener, Monitor
{
public:
	ProcessMonitor(Visitor *v);
	ProcessMonitor(void);
	~ProcessMonitor(void);

	/*
		Function: Start

		Creates a new thread <procmonThread>
	*/
	void Start();
	/*
		Function: Stop

		Stops the thread stored in <procmonThread>
	*/
	void Stop();
	/*
		Function: Run

		Runnable thread which waits for <hEvent> to be signalled. This means that a process event has been stored in the
		kernel driver and is ready to be retrieved. It retrieves it, parses it, checks if it is exluded and notifies any
		listeners of the process event.
	*/
	void Run();
	/*
		Function: Pause

		Pauses the process monitor kernel driver so it does not listen for system wide process events
	*/
	void Pause();
	/*
		Function: UnPause

		Unpauses the process monitor kernel driver and starts listening for system wide process events
	*/
	void UnPause();

	/* Event methods */
	void AddProcessListener(ProcessListener* pl);
	void RemoveProcessListener(ProcessListener* pl);
	void NotifyListeners(BOOLEAN bCreate, wstring time, wstring processParentPath, wstring processPath);

	/* VisitorListener Interface */
	void OnVisitEvent(int eventType, DWORD minorEventCode, wstring url, wstring visitor);	
private:
	/*
		Function: CheckIfExcluded

		[Private] Checks if the current process is excluded
	*/
	bool CheckIfExcluded(wstring processPath)
	{
		if(processPath == currentVisitingClient)
			return true;
		return false;
	}

	/*
		Function: GetProcessCompletePathName

		[Private] Gets the complete path name of a process from its id
	*/
	bool GetProcessCompletePathName(DWORD pid, wstring* path, bool created);

	/*
		Variable: procmonThread
	*/
	Thread* procmonThread;
	/*
		Variable: hDriver
	*/
	HANDLE hDriver;
	/*
		Variable: hEvent
	*/
	HANDLE hEvent;
	/*
		Variable: processListeners
	*/
	std::list<ProcessListener*> processListeners;
	/*
		Variable: currentVisitingClient
	*/
	wstring currentVisitingClient;
	/*
		Variable: installed
	*/
	bool installed;
	
};
