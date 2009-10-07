/*
*	PROJECT: Capture 3.0
*	FILE: KernelDriver.cpp
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

#include "Precompiled.h"

#include "KernelDriver.h"

wchar_t* KernelDriver::debug_privilege	= L"SeDebugPrivilege";
wchar_t* KernelDriver::load_driver_privilege =	L"SeLoadDriverPrivilege";

KernelDriver::KernelDriver(void)
{
	driver_pipe_opened = false;
	installed = false;

	driver_pipe_handle = INVALID_HANDLE_VALUE;
}

KernelDriver::~KernelDriver(void)
{
	uninstall();
}

bool
KernelDriver::install(const std::wstring& driver_path, const std::wstring& driver_name, const std::wstring& driver_description)
{
	SC_HANDLE service_manager_handle;

    service_manager_handle = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

    if(service_manager_handle)
    {
        kernel_driver_handle = CreateService(service_manager_handle, driver_name.c_str(), 
								  driver_description.c_str(), 
                                  SERVICE_START | DELETE | SERVICE_STOP, 
                                  SERVICE_KERNEL_DRIVER,
                                  SERVICE_DEMAND_START, 
                                  SERVICE_ERROR_IGNORE, 
								  driver_path.c_str(), 
                                  NULL, NULL, NULL, NULL, NULL);

        if(!kernel_driver_handle)
        {
			kernel_driver_handle  = OpenService(service_manager_handle, driver_name.c_str(), 
                       SERVICE_START | DELETE | SERVICE_STOP);
        }

        if(kernel_driver_handle )
        {
            if(StartService(kernel_driver_handle, 0, NULL))
			{
				LOG(INFO, "KernelDriver: Loaded kernel driver: %ls\n", driver_name.c_str());
				installed = true;
			} else {
				DWORD err = GetLastError();
				if(err == ERROR_SERVICE_ALREADY_RUNNING)
				{
					LOG(INFO, "KernelDriver: Driver already loaded: %ls\n", driver_name.c_str());
					installed = true;
				} else {
					LOG(ERR, "KernelDriver: Error loading kernel driver: %ls - 0x%08x\n", driver_name.c_str(), err);
				}
			}
		} else {
			LOG(ERR, "KernelDriver: Error loading kernel driver: %ls - 0x%08x\n", driver_name.c_str(), GetLastError());
		}
        CloseServiceHandle(service_manager_handle);	
    }
	return installed;
}

bool
KernelDriver::open(const std::wstring& pipe_name)
{
	driver_pipe_handle = CreateFile(
				pipe_name.c_str(),
				GENERIC_READ | GENERIC_WRITE, 
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				0,                     // Default security
				OPEN_EXISTING,
				FILE_FLAG_OVERLAPPED,  // Perform asynchronous I/O
				0);                    // No template
	
	if(driver_pipe_handle == INVALID_HANDLE_VALUE)
	{
		LOG(ERR, "KernelDriver: cannot open pipe: %ls - 0x%08x\n", pipe_name.c_str(), GetLastError());
		return false;
	} else {
		driver_pipe_opened = true;
		return true;
	}
}

bool
KernelDriver::uninstall()
{
	//if(installed)
	//{
		SERVICE_STATUS ss;
		ControlService(kernel_driver_handle, SERVICE_CONTROL_STOP, &ss);
		DeleteService(kernel_driver_handle);
		CloseServiceHandle(kernel_driver_handle);
		installed = false;
	//}
	return installed;
}

bool 
KernelDriver::receiveData(unsigned long io_control_code, unsigned char* data_buffer, unsigned long data_buffer_size, unsigned long& received_bytes)
{
	if(!installed)
		return false;

	return sendAndReceiveData(io_control_code, NULL, NULL, data_buffer, data_buffer_size, received_bytes);
}

bool 
KernelDriver::sendData(unsigned long io_control_code, unsigned char* data_buffer, unsigned long data_buffer_size)
{
	if(!installed)
		return false;

	unsigned long received_bytes; // Not used
	return sendAndReceiveData(io_control_code, data_buffer, data_buffer_size, NULL, NULL, received_bytes);
}

bool 
KernelDriver::sendAndReceiveData(unsigned long io_control_code, 
										unsigned char* send_data_buffer, unsigned long send_data_buffer_size, 
										unsigned char* receive_data_buffer, unsigned long receive_data_buffer_length, unsigned long& received_bytes)
{
	if(!installed && driver_pipe_opened)
		return false;

	unsigned long blah = 0;

	BOOL success = DeviceIoControl(driver_pipe_handle,
		io_control_code, 
		send_data_buffer, 
		send_data_buffer_size, 
		receive_data_buffer, 
		receive_data_buffer_length,
		&received_bytes, 
		NULL);

	if(!success)
	{
		DWORD err = GetLastError();
		// TODO do something with error
		return false;
	}
	return true;
}

bool
KernelDriver::acquirePrivilege( const wchar_t* privilege )
{
	LUID luid;
	HANDLE handle_token;
	bool acquired = false;
	
	// Open the current processes token so that we can adjust its privileges
	if( !OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &handle_token ) )
	{
		LOG(ERR, "KernelDriver: cannot open process token - %08x\n", GetLastError());
		return false;
	}

	// Find the specified privilege
	if ( LookupPrivilegeValue( NULL, privilege, &luid ) )
	{
		TOKEN_PRIVILEGES token_privileges;

		token_privileges.PrivilegeCount = 1;
		token_privileges.Privileges[0].Luid = luid;
		token_privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		// Enable the privilege
		if ( AdjustTokenPrivileges(handle_token, FALSE, &token_privileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL) )
		{ 
			acquired = true;
		}
		else
		{
			LOG(ERR, "KernelDriver: cannot acquire privilege - %ls - %08x\n", privilege, GetLastError());
		}
	}
	else
	{
		LOG(ERR, "KernelDriver: %ls privilege not found - %08x\n", privilege, GetLastError());
	}

	CloseHandle( handle_token );

	return acquired;
}