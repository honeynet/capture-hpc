/*
 *	PROJECT: Capture
 *	FILE: FileMonitor.h
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
#include "stdafx.h"
#include <windows.h>
#include <winioctl.h>
#include "Thread.h"
#include "Monitor.h"
#include "Psapi.h"
#include "MiniFilter.h"

typedef pair <wstring, wstring> Dos_Pair;

/*
   Constants: File Monitor Communication Port Name

   CAPTURE_FILEMON_PORT_NAME - Contains the device path to the communications port that the file monitor kernel driver creates
*/
#define CAPTURE_FILEMON_PORT_NAME	L"\\CaptureFileMonitorPort"

/*
	Enum: FILEMONITOR_COMMAND

	Commands that are sent through <commPort> to the kernel driver

	GetFileEvents - Retrieve a buffer that contains a list of <FILE_EVENT>'s
	FStart - Start monitoring for file events
	FStop - Stop monitoring for file events
*/
typedef enum _FILEMONITOR_COMMAND {

    GetFileEvents,
	FStart,
	FStop

} FILEMONITOR_COMMAND;

/*
	Enum: FILE_NOTIFY_CLASS

	Enum containing the types of events that the kernel driver is monitoring

	FilePreRead - File read event type
	FilePreWrite - File write event type
*/
typedef enum _FILE_NOTIFY_CLASS {
    FilePreRead,
    FilePreWrite
} FILE_NOTIFY_CLASS;

/*
	Struct: FILEMONITOR_MESSAGE

	Contains a command to be sent through to the kernel driver

	Command - Actual command to be sent
*/
typedef struct _FILEMONITOR_MESSAGE {
    FILEMONITOR_COMMAND Command;
} FILEMONITOR_MESSAGE, *PFILEMONITOR_MESSAGE;

/*
	Struct: FILE_EVENT

	File event that contains what event happened on what file and by what process

	type - file event type of <FILE_NOTIFY_CLASS>
	time - time when the event occured in the kernel
	name - Contains the name of the event (file modified)
	processID - The process id that caused the file event
*/
typedef struct  _FILE_EVENT {
	FILE_NOTIFY_CLASS type;
	TIME_FIELDS time;
	WCHAR name[1024];
	DWORD processID;
} FILE_EVENT, *PFILE_EVENT;

/*
	Class: FileListener
	
	Interface for objects to implement so that they can be notified when file events are received
*/
class FileListener
{
public:
	/*
		Function: OnFileEvent
	*/
	virtual void OnFileEvent(wstring eventType, wstring time, wstring processFullPath, wstring filePath) = 0;
};

/*
	Class: FileMonitor

	File monitor class that communicated with the minifilter driver using the <commPort>. Every 100ms the <Run> thread
	checks <commPort> and passes a userspace buffer to the kernel driver which copies <FILE_EVENT>'s into it which is
	then returned to userspace where they are excluded and reported to the objects which registered to be notified when
	file events occur.

	Implements: <IRunnable>, <Monitor>
*/
class FileMonitor : public IRunnable, public Monitor
{
public:
	FileMonitor();
	~FileMonitor(void);
	
	/*
		Function: Start

		Creates a new thread <filemonThread> that monitors the <commPort> for incoming data
	*/
	void Start();
	/*
		Function: Stop

		Stops the thread stored in <filemonThread>
	*/
	void Stop();
	/*
		Function: Run

		Runnable thread which monitors the <commPort> for messages, retrieves them, parses them to events and checks if
		they are excluded before passing the event onto the listeners. If a message is passed it is considered malicious.
	*/
	void Run();
	/*
		Function: Pause

		Pauses the file monitor kernel driver so it does not listen for system wide file events
	*/
	void Pause();
	/*
		Function: UnPause

		Unpauses the file monitor kernel driver and starts listening for system wide file events
	*/
	void UnPause();

	/*
		Function: AddFileListener

		Add an object that implements the <FileListener> interface so that it can be notified of file events
	*/
	void AddFileListener(FileListener* rl);
	/*
		Function: RemoveFileListener

		Remove an object that is listening for file events
	*/
	void RemoveFileListener(FileListener* rl);
	/*
		Function: NotifyListeners

		Notify all objects that are listening of a file event
	*/
	void NotifyListeners(wstring eventType, wstring time, wstring processFullPath, wstring eventPath);

private:
	/*
		Variable: filemonThread

		<FileMonitor> implements <IRunnable> so that it can monitor for file events in a thread. This contains a pointer
		to the thread class that holds this.
	*/
	Thread* filemonThread;
	/*
		Variable: hDriver

		A handle to the file monitor kernel driver so that a user space process can interact with it
	*/
	HANDLE hDriver;
	/*
		Variable: commPort

		Communications port that minifilter drivers use to communicate to and from userspace and kernelspace
	*/
	HANDLE commPort;
	/*
		Variable: installed

		Boolean that tells whether or not the kernel driver is successfully loaded
	*/
	bool installed;
	/*
		Variable: fileListeners

		Linked list containing a list of objects that are listening for file events
	*/
	std::list<FileListener*> fileListeners;
	/*
		Variable: dosNameMap

		Maps a logical string \DEVICE\* to its DOS drive such as c, or d drive ... etc
	*/
	stdext::hash_map<wstring, wstring> dosNameMap;
};