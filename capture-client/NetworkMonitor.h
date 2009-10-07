/*
 *	PROJECT: Capture
 *	FILE: NetworkMonitor.h
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
#include "CaptureGlobal.h"
#include "Monitor.h"
#include "Thread.h"
#include "Element.h"

#include <boost/signal.hpp>

typedef struct _KernelEventList			KernelEventList;

/*
	Class: NetworkMonitor

	Manages the CaptureConnectionMonitor kernel driver. It waits for a kernel event to
	be singalled which tells it that a connection event has occured. This is retrieved
	from kernel space and parsed into an event which is then checked for exclusion and
	passed onto all objects which are attached to the onNetworkEvent slot

	Implements: <IRunnable>, <VisitorListener>, <Monitor>
*/
class NetworkMonitor : public Runnable, public Monitor
{
public:
	typedef boost::signal<void (const std::wstring&, const std::wstring&, const DWORD, const std::wstring&, const std::wstring&)> signal_connectionEvent;
public:
	NetworkMonitor(void);
	virtual ~NetworkMonitor(void);

	void start();
	void stop();
	void run();

	void processTcpEvents(KernelEventList* tcp_event_list);
	void processUdpEvents(KernelEventList* udp_event_list);

	void onNetworkExclusionReceived(const Element& pElement);

	boost::signals::connection connect_onConnectionEvent(const signal_connectionEvent::slot_type& s)
	{
		return signal_onConnectionEvent.connect(s); 
	}
private:
	signal_connectionEvent signal_onConnectionEvent;


	HANDLE event_count[2];
	HANDLE driver_handle;
	Thread* monitor_thread;
	bool driver_installed;
	bool monitor_running;
	//boost::signals::connection networkManagerConnection;
	//boost::signals::connection onNetworkExclusionReceivedConnection;
};
