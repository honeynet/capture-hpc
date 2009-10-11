/*
*	PROJECT: Capture
*	FILE: SharedBuffer.h
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

#include <ntddk.h>

/// SharedBuffer
/// A buffer of a certain size that can be shared and accessed in
/// kernel space and user space
///
/// The kernel buffer will point to a NON-PAGED block of memory
/// The shared buffer is mapped into the userspace calling process
/// so InitialiseSharedBuffer must be run a PASSIVE_LEVEL
///
/// I'm not sure if this method is very secure so use this at your own risk

typedef struct _SharedBuffer SharedBuffer;

struct _SharedBuffer
{
	// Pointer to the userspace buffer. Valid only in the context of the user process id
	PVOID u_buffer;

	// Pointer the the kernel space buffer. Accessible everywhere in kernel space
	PVOID k_buffer;

	// Memory descriptor describing the allocation
	PMDL mdl;

	// Process id where the userspace buffer is mapped
	ULONG u_process_id;
};

/// Initialise the shared buffer
/// @param	The shared buffer to initialise
/// @param	Size of the buffer to allocate in bytes
BOOLEAN InitialiseSharedBuffer(SharedBuffer* shared_buffer, unsigned long size);

/// Destroy the shared buffer
/// @param	The shared buffer the destroy
VOID DestroySharedBuffer(SharedBuffer* shared_buffer);

BOOLEAN MapUserSpaceSharedBuffer(SharedBuffer* shared_buffer);