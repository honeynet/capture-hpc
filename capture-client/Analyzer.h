/*
 *	PROJECT: Capture
 *	FILE: Analyzer.h
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
#include "ProcessMonitor.h"
#include "Visitor.h"
#include "ServerConnection.h"
#include "RegistryMonitor.h"
#include "FileMonitor.h"




/*
	Class: Analyzer

	The analyzer is the central part of the client component of Capture. It creates all of the objects necessary for monitoring
	the system and maintaing a connection to the server. It implements most of the Listener interfaces avaiable so that it can
	be notified when a particular event is receieved. All events that are received inside the <OnVisitEvent>, <OnProcessEvent>, and
	<OnRegistryEvent> are malicious as the monitors themselves take care of exclusions. Only one call to each On* function occurs at
	a time however there maybe multiple different On* functions being called at a time. This is protected by the <ServerConnection::Send>
	function which only allows one thread to access it.
*/
class Analyzer : public ProcessListener, VisitorListener, RegistryListener, FileListener, ServerListener
{
public:
	/*
		Constructor: Analyzer

		Initiates a server connection if required and Start all of the monitors and sets them to a paused state. If no
		server connection is required, all the monitors are unpaused and the analyzer will print out all events onto
		the console. Registers itself with all the monitors so that it can be informed of all the events that can occur
		in a system.
	*/
	Analyzer(int argc, CHAR* argv[]);

	/*
		Function: OnProcessEvent

		Called when a malicious process event occurs. Sends the event to the server
	*/
	void OnProcessEvent(BOOLEAN bCreate, wstring time, wstring processParentPath, wstring processPath);

	/*
		Function: OnVisitEvent

		Called when a visit event occurs. Sends the "visit::url" and "visited::url" messages to the server
	*/
	void OnVisitEvent(int eventType, DWORD minorEventType, wstring url, wstring visitor);	
	/*
		Function: OnRegistryEvent

		Called when a malicious registry event occurs. Sends the event to the server
	*/
	void OnRegistryEvent(wstring eventType, wstring time, wstring processFullPath, wstring registryPath);
	/*
		Function: OnFileEvent

		Called when a malicious file event occurs. Sends the event to the server
	*/
	void OnFileEvent(wstring eventType, wstring time, wstring processFullPath, wstring filePath);
	/*
		Function: OnReceiveEvent

		The analyzer registers itself with the <ServerConnection> to receive "connect::" and "ping::" messages from
		the server. A "connect::" message is sent back to the server which will contain the unique id of the VMware Server
		this VM is being run on, and the VM id it is in. These are passed in the command line upon starting Capture. A "pong::"
		message is sent when a "ping::" message is sent
	*/
	void OnReceiveEvent(vector<wstring> e);
public:
	~Analyzer(void);

private:
	/*
		Function: LocalTimeToWString
	*/
	wstring LocalTimeToWString(SYSTEMTIME time)
	{
		wchar_t szTime[16];
		ZeroMemory(&szTime, sizeof(szTime));
		wstring wtime;
		_itow_s(time.wDay,szTime,16,10);
		wtime += szTime;
		wtime += L"\\";
		_itow_s(time.wMonth,szTime,16,10);
		wtime += szTime;
		wtime += L"\\";
		_itow_s(time.wYear,szTime,16,10);
		wtime += szTime;
		wtime += L" ";
		_itow_s(time.wHour,szTime,16,10);
		wtime += szTime;
		wtime += L":";
		_itow_s(time.wMinute,szTime,16,10);
		wtime += szTime;
		wtime += L":";
		_itow_s(time.wSecond,szTime,16,10);
		wtime += szTime;
		wtime += L".";
		_itow_s(time.wMilliseconds,szTime,16,10);
		wtime += szTime;
		return wtime;
	}
	/*
		Function: ObtainDriverLoadPrivilege

		[Private] Obtains the SeLoadDriver privilege for the current process which is required to load the kernel drivers
	*/
	bool ObtainDriverLoadPrivilege();	
	/*
		Function: ObtainDebugPrivilege

		[Private] Obtains the SeDebugPrivilege privilege for the current process which is required to do something
	*/
	bool ObtainDebugPrivilege();
	/*
		Variable: serverId

		Contains the unique id of the VMware Server that was passed during startup
	*/
	char* serverId;
	/*
		Variable: vmId

		Contains the unique id of the VM that contains the guest os. Passed during startup
	*/
	char* vmId;
	/*
		Variable: procMon

		Pointer to a <ProcessMonitor>
	*/
	ProcessMonitor* procMon;
	/*
		Variable: regMon

		Pointer to a <RegistryMonitor>
	*/
	RegistryMonitor* regMon;
	/*
		Variable: fileMon

		Pointer to a <FileMonitor>
	*/
	FileMonitor* fileMon;
	Common* global;
	/*
		Variable: visitor

		Pointer to a <Visitor>
	*/
	Visitor* visitor;
	/*
		Variable: serverConnection

		Pointer to a <ServerConnection>
	*/
	ServerConnection* serverConnection;
	/*
		Variable: malicious

		Whether or not the visiting url is malicious or not
	*/
	bool malicious;
	/*
		Variable: connected

		Whether or not the client is connected to a server
	*/
	bool connected;
};
