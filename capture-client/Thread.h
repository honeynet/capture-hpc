/*
 *	PROJECT: Capture
 *	FILE: Thread.h
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
/*
	Class: Thread

	Class that accepts a pointer to another class that implements the <IRunnable> interface. This then wraps the c-style function 
	for creating a thread (threadProc) and creates an OO way to initialise, start, and stop a thread.
*/
/*
	Interface: IRunnable

	Workaround to create an OO-style thread
*/
struct IRunnable {
  virtual void Run() = 0;
};

class Thread {
public:
	/*
		Constructor: Thread

		Creates a thread object and stores the object which implements the IRunnable interface in <_threadObj>
	*/
	Thread(IRunnable *ptr) {
		_threadObj = ptr;
	}

	/*
		Function: Start

		Starts a thread
	*/
	DWORD Start() {
		DWORD threadID;
		hThread = CreateThread(0, 0, threadProc, _threadObj, 0, &threadID);
		return threadID;
	}

	/*
		Function: Stop

		Stops the running thread that was created
	*/
	BOOL Stop() {
		if(hThread != NULL)
			return TerminateThread(hThread, 0);
		return FALSE;
	}
  
protected:
	/*
		Variable: hThread

		Handle to the thread when <Start> is called
	*/
	HANDLE hThread;

	/*
		Variable: _threadObj

		Pointer to an object which implements the IRunnable interface
	*/
	IRunnable *_threadObj;

	/*
		Function: threadProc

		Static c-function which can be used in the CreateThread function in windows.h
	*/
	static unsigned long __stdcall threadProc(void* ptr) {
		((IRunnable*)ptr)->Run();
		return 0;
	}   
};

