/*
*	PROJECT: Capture
*	FILE: EventList.c
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

#include "EventList.h"

/// Initialises the manager for the buffer
void InitialiseBufferManager( Buffer* buffer )
{
	ULONG free = 0;

	//DbgBreakPoint();

	RtlZeroMemory( buffer, sizeof(Buffer) );

	RtlInitializeBitMap( &buffer->bitmap, buffer->buffer_bitmap, BUFFER_BITMAP_SIZE * (sizeof(ULONG)*8) );

	free = RtlNumberOfClearBits( &buffer->bitmap );
}

/// Reserve space in a buffer
/// @param	buffer	The buffer to reserver space in
/// @param	size	The amount of space to reserve
///
/// @return			A pointer to a block of memory of size, or null if not enough free space
PVOID ReserveSpace( Buffer* buffer, ULONG size )
{
	ULONG bits = 0;
	ULONG position = 0;
	PVOID data_buffer = NULL;

	// Align size to BLOCK_SIZE
	bits = ALIGN_UP(size, Block) / BLOCK_SIZE;

	// Start Lock

	position = RtlFindClearBitsAndSet( &buffer->bitmap, bits, buffer->bitmap_hint_index );

	if( position != 0xFFFFFFFF )
	{
		// Cache the previous position for faster lookups
		buffer->bitmap_hint_index = position;

		data_buffer = buffer->buffer + (position * BLOCK_SIZE);
	}
	else
	{
		DbgPrint("Buffer %08x: ERROR - Out of free space\n");
	}

	// End Lock

	return data_buffer;
}

/// Intialise the shared event list
BOOLEAN InitialiseSharedEventList( SharedEventList* shared_event_list )
{
	BOOLEAN success = FALSE;

	success = InitialiseSharedBuffer(&shared_event_list->shared_buffer, sizeof(EventList));

	if(!success)
		return FALSE;

	shared_event_list->semaphore = NULL;

	shared_event_list->event_list = (EventList*)shared_event_list->shared_buffer.k_buffer;

	InitialiseBufferManager(&shared_event_list->event_list->buffer);

	return TRUE;
}

/// Destroy the shared event list
void DestroySharedEventList( SharedEventList* shared_event_list )
{
	DestroySharedBuffer(&shared_event_list->shared_buffer);

	if(shared_event_list->semaphore != NULL)
	{
		ObDereferenceObject(shared_event_list->semaphore);
	}

	RtlZeroMemory(shared_event_list, sizeof(SharedEventList));
}

/// Setup the shared semaphore passed from userspace, stored in the IRP IOCTL buffer
/// Call this when GET_SHARED_EVENT_LIST_IOCTL is received
NTSTATUS GetSharedEventListIoctlHandler(SharedEventList* shared_event_list, PDEVICE_OBJECT device_object, PIRP irp, PIO_STACK_LOCATION irp_stack)
{
	ULONG process_id = 0;
	PVOID input_buffer = NULL;
	PVOID output_buffer = NULL;
	ULONG input_buffer_length = 0;
	ULONG output_buffer_length = 0;
	HANDLE semaphore_handle = NULL;

	process_id = (ULONG)PsGetCurrentProcessId();

	output_buffer = irp->UserBuffer;
	input_buffer = irp_stack->Parameters.DeviceIoControl.Type3InputBuffer;

	input_buffer_length = irp_stack->Parameters.DeviceIoControl.InputBufferLength;
	output_buffer_length = irp_stack->Parameters.DeviceIoControl.OutputBufferLength;

	if(!MapUserSpaceSharedBuffer(&shared_event_list->shared_buffer))
	{
		DbgPrint("GetSharedEventListIoctlHandler: ERROR - couldn't map share buffer into userspace VM\n");
		return STATUS_SEVERITY_ERROR;
	}

	if(input_buffer_length < sizeof(HANDLE))
	{
		DbgPrint("GetSharedEventListIoctlHandler: ERROR - input buffer not big enough to contain a handle to a semaphore\n");
		return STATUS_BUFFER_TOO_SMALL;
	}

	// Make sure we were passed enough input space to send the 
	// event list pointer back to user space
	if(output_buffer_length < sizeof(unsigned long))
	{
		DbgPrint("GetSharedEventListIoctlHandler: ERROR - output buffer not big enough to store shared event list pointer\n");
		return STATUS_BUFFER_TOO_SMALL;
	}

	// TODO use buffered I/O rather than neither, or something that doesn't
	// require probing as its pointless doing it for 4 bytes of memory
	__try
	{
		// Make sure we can write to the buffer supplied
		ProbeForWrite(irp->UserBuffer, irp_stack->Parameters.DeviceIoControl.OutputBufferLength, __alignof( unsigned long ) );
		ProbeForRead(irp_stack->Parameters.DeviceIoControl.Type3InputBuffer, irp_stack->Parameters.DeviceIoControl.InputBufferLength, __alignof(unsigned long));
	} 
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("GetSharedEventListIoctlHandler: ERROR - unable to access output buffer\n");
		return STATUS_SEVERITY_ERROR;
	}

	if(input_buffer == NULL)
		return STATUS_BUFFER_TOO_SMALL;

	if(output_buffer == NULL)
		return STATUS_BUFFER_TOO_SMALL;

	// Get the handle passed from userspace and reference it through the object manager which will
	// give us our kernel semaphore object to use in kernel space
	semaphore_handle = (HANDLE)input_buffer;
	ObReferenceObjectByHandle(semaphore_handle, 0, *ExSemaphoreObjectType, UserMode, &shared_event_list->semaphore, NULL);

	// Copy the pointer user pointer to the shared buffer to the output buffer
	// This pointer is mapped into the address space of the calling process already
	// so is valid in that context
	RtlCopyMemory(output_buffer, &shared_event_list->shared_buffer.u_buffer, sizeof(unsigned long));
	irp->IoStatus.Information = sizeof(unsigned long);
	irp->IoStatus.Status = STATUS_SUCCESS;

	return STATUS_SUCCESS;
}

/// Create an event
Event* CreateEvent( SharedEventList* shared_event_list )
{
	Event* event = NULL;
	EventList* event_list = shared_event_list->event_list;

	// Reserves space in the buffer to hold an event
	event = (Event*)ReserveSpace( &event_list->buffer, sizeof(Event) );

	if(event == NULL)
		return NULL;

	RtlZeroMemory( event, sizeof(Event) );

	return event;
}

/// Insert event into the event list
void InsertEvent( SharedEventList* shared_event_list, Event* event )
{
	EventList* event_list = shared_event_list->event_list;

	event->ready = 1;

	// Start Lock

	event->previous = shared_event_list->event_list->tail;

	if( event_list->head == (ULONG)NULL )	event_list->head = ADDRESS_TO_OFFSET(event_list, event);

	if( event_list->tail != (ULONG)NULL )	OFFSET_TO_POINTER(Event*, event_list, event_list->tail)->next = ADDRESS_TO_OFFSET(event_list, event);

	event_list->tail = ADDRESS_TO_OFFSET(event_list, event);

	if(shared_event_list->semaphore)
		KeReleaseSemaphore(shared_event_list->semaphore, 0, 1, FALSE);

	// End Lock
}

/// Add data to and event
/// @param	event_list	Event list to allocate space for the data from
/// @param	event		The event to allocate data for
/// @param	key			A key hash of the data
/// @param	type		The type of data being stored
/// @param	size		The size of the data being stored
/// @param	data		The data to be copied into the event
///
/// @return				A pointer to a DataContainer containing the copied data or null if an error
DataContainer* AddEventData( SharedEventList* shared_event_list, Event* event, ULONG key, ULONG type, ULONG size, PVOID data )
{
	DataContainer* data_container = NULL;
	EventList* event_list = shared_event_list->event_list;

	// Reserve enough space for the data container and the data being stored inside of it
	data_container = (DataContainer*)ReserveSpace( &event_list->buffer, sizeof(DataContainer) + size );

	if(data_container == NULL)
		return NULL;

	RtlZeroMemory( data_container, sizeof(DataContainer) );

	data_container->key = key;
	data_container->type = type;
	data_container->size = size;

	// Copy the data into the container
	if( data )
	{
		RtlCopyMemory( data_container->data, data, size );
	}

	// Insert the data into the events data linked list
	if( event->head == (ULONG)NULL ) event->head = ADDRESS_TO_OFFSET(event_list, data_container);

	if( event->tail != (ULONG)NULL ) OFFSET_TO_POINTER(DataContainer*, event_list, event->tail)->next = ADDRESS_TO_OFFSET(event_list, data_container);

	event->tail = ADDRESS_TO_OFFSET(event_list, data_container);

	return data_container;
}

/// Add data to and event
/// Functionally equivalent to AddEventData but no data is specified at the moment so the container is not filled
DataContainer* ReserveEventData( SharedEventList* shared_event_list, Event* event, ULONG key, ULONG type, ULONG size )
{
	return AddEventData( shared_event_list, event, key, type, size, NULL );
}