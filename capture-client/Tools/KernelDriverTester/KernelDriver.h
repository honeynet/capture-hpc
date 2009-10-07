/*
*	PROJECT: Capture 3.0
*	FILE: KernelDriver.h
*	AUTHORS: Ramon Steenson (rsteenson@gmail.com)
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
#include "Precompiled.h"

/// A wrapper around a windows HANDLE to a server
class KernelDriver
{
public:
	KernelDriver(void);
	virtual ~KernelDriver(void);

	bool install(const std::wstring& driver_path, const std::wstring& driver_name, const std::wstring& driver_description);
	bool uninstall();

	bool open(const std::wstring& pipe_name);

	// Send an IOCTL and receive data from a kernel driver
	bool receiveData(unsigned long io_control_code, unsigned char* data_buffer, unsigned long data_buffer_size, unsigned long& received_bytes);
	
	// Send an IOCTL and send data to a kernel driver
	bool sendData(unsigned long io_control_code, unsigned char* data_buffer, unsigned long data_buffer_size);
	
	// Send and receive data from a kernel driver
	bool sendAndReceiveData(unsigned long io_control_code, 
		unsigned char* send_data_buffer, unsigned long send_data_buffer_size, 
		unsigned char* receive_data_buffer, unsigned long receive_data_buffer_length, unsigned long& received_bytes);

	inline bool isInstalled() { return installed; }

public:
	static wchar_t* debug_privilege;
	static wchar_t* load_driver_privilege;

	// Give debugging privileges so it can inspect other processes
	static bool acquirePrivilege(const wchar_t* privilege);
private:
	std::wstring driver_path;
	std::wstring driver_name;
	std::wstring driver_description;

	bool installed;
	bool driver_pipe_opened;

	SC_HANDLE kernel_driver_handle;
	HANDLE driver_pipe_handle;
};
