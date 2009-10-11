/*
*	PROJECT: Capture
*	FILE: SharedBuffer.c
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

#include "SharedBuffer.h"

/// Destroy the shared buffer
/// @param	The shared buffer the destroy
VOID DestroySharedBuffer(SharedBuffer* shared_buffer)
{
	if(shared_buffer->u_buffer != NULL)
		MmUnmapLockedPages(shared_buffer->u_buffer, shared_buffer->mdl);
	if(shared_buffer->k_buffer != NULL)
		MmUnmapLockedPages(shared_buffer->k_buffer, shared_buffer->mdl);

	MmFreePagesFromMdl(shared_buffer->mdl);     

	ExFreePool(shared_buffer->mdl);
}

/// Initialise the shared buffer
/// @param	The shared buffer to initialise
/// @param	Size of the buffer to allocate in bytes
BOOLEAN InitialiseSharedBuffer(SharedBuffer* shared_buffer, unsigned long size)
{
	PHYSICAL_ADDRESS low_address;
	PHYSICAL_ADDRESS high_address;

	// Range to allocate the buffer in. Currently anywhere
	low_address.QuadPart = 0;
	high_address.QuadPart = (ULONGLONG)-1;

	shared_buffer->mdl = MmAllocatePagesForMdl(low_address, high_address, low_address, size);

	if(shared_buffer->mdl == NULL)
	{
		return FALSE;
	}

	_try 
	{
		shared_buffer->k_buffer = MmMapLockedPagesSpecifyCache(shared_buffer->mdl, KernelMode, MmCached, NULL, FALSE, NormalPagePriority);
	} 
	_except (EXCEPTION_EXECUTE_HANDLER)	// UserMode access mode can throw an exception
	{
	}

	if(shared_buffer->k_buffer == NULL)
	{
		DestroySharedBuffer(shared_buffer);

		return FALSE;
	}

	return TRUE;
}

BOOLEAN MapUserSpaceSharedBuffer(SharedBuffer* shared_buffer)
{
	if(shared_buffer->u_buffer)
		return FALSE;

	shared_buffer->u_process_id = (ULONG)PsGetCurrentProcessId();

	DbgPrint("SharedBuffer mapped into process %i\n", shared_buffer->u_process_id);

	_try 
	{
		shared_buffer->u_buffer = MmMapLockedPagesSpecifyCache(shared_buffer->mdl, UserMode, MmCached, NULL, FALSE, NormalPagePriority);
	} 
	_except (EXCEPTION_EXECUTE_HANDLER)	// UserMode access mode can throw an exception
	{
		return FALSE;
	}

	return TRUE;
}