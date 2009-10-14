/*
*	PROJECT: Capture
*	FILE: EventList.h
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

/// SharedEventList
/// An event list that is a contigious block of memory that is shared between 
/// userspace and kernel space.
/// 
/// Events and Data are stored in a buffer that is managed by a BitMap. The
/// EventList, Events, and Data are entirely stored in the shared buffer,
/// this means that pointers such as the list of events or data are stored
/// as offsets rather than pointers with the base address being the EventList 
/// itself. This allows the EventList to be shared between userspace and 
/// kernelspace.
///
/// Please note that the synchronisation is not yet complete. And there also
/// maybe a security risk using this technique.

#include "SharedBuffer.h"

#define GET_SHARED_EVENT_LIST_IOCTL		CTL_CODE( 0x00000022, 0x810, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA )

#define ALIGNUP(length, size)	((ULONG)(length) & ~(size - 1))
#define ALIGN(x,a) ((typeof(x))	(((x) + (a) - 1) & ~((typeof(x)) (a) - 1)))

#define ADDRESS_TO_OFFSET(base, address) ((ULONG)((ULONG_PTR)address - (ULONG_PTR)base))
#define OFFSET_TO_POINTER(object, base, address) ((object)((PUCHAR)address + (ULONG)base))

typedef unsigned char	BYTE;
typedef ULONG			OFFSET;

typedef struct _Block				Block;
typedef struct _Buffer				Buffer;
typedef struct _DataContainer		DataContainer;
typedef struct _Event				Event;
typedef struct _EventList			EventList;
typedef struct _SharedEventList		SharedEventList;

/// Block
/// A block of memory
struct _Block
{
	BYTE data[16];
};

#define BUFFER_SIZE				512 * 1024
#define BLOCK_SIZE				sizeof(Block)
#define BUFFER_BITMAP_SIZE		(BUFFER_SIZE / (sizeof(ULONG)*BLOCK_SIZE*8))

/// Buffer Manager
/// Manages which blocks are free/used in a buffer
struct _Buffer
{
	RTL_BITMAP	bitmap;
	KSPIN_LOCK	bitmap_lock;
	ULONG		bitmap_hint_index;
	ULONG		buffer_bitmap[BUFFER_BITMAP_SIZE];
	BYTE		buffer[BUFFER_SIZE];
};

/// DataContainer
/// A variable size structure holding data of a certain type
struct _DataContainer
{
	OFFSET		next;
	long		key;
	long		type;
	long		size;
	BYTE		data[];
};

/// Event
/// An event containing a linked list of DataContainers
/// NOTE: OFFSET's are pointer offsets from the base buffer pointer of the EventList
struct _Event
{
	OFFSET		next;
	OFFSET		previous;
	long		ready;
	OFFSET		head;
	OFFSET		tail;
};

struct _EventList
{
	OFFSET			head;
	OFFSET			tail;

	Buffer			buffer;
};

struct _SharedEventList
{
	EventList*		event_list;

	KSPIN_LOCK		event_list_lock;

	// How many events are stored in the shared list
	PKSEMAPHORE		semaphore;

	SharedBuffer	shared_buffer;
};

/// Intialise the shared event list
BOOLEAN InitialiseSharedEventList( SharedEventList* shared_event_list );

/// Destroy the shared event list
void DestroySharedEventList( SharedEventList* shared_event_list );

/// Setup the shared semaphore passed from userspace, stored in the IRP IOCTL buffer
/// Call this when GET_SHARED_EVENT_LIST_IOCTL is received
NTSTATUS GetSharedEventListIoctlHandler(SharedEventList* shared_event_list, PDEVICE_OBJECT device_object, PIRP irp, PIO_STACK_LOCATION irp_stack);

/// Create an event
Event* CreateEvent( SharedEventList* shared_event_list );

/// Insert event into the event list
void InsertEvent( SharedEventList* shared_event_list, Event* event );

/// Add data to and event
/// @param	event_list	Event list to allocate space for the data from
/// @param	event		The event to allocate data for
/// @param	key			A key hash of the data
/// @param	type		The type of data being stored
/// @param	size		The size of the data being stored
/// @param	data		The data to be copied into the event
///
/// @return				A pointer to a DataContainer containing the copied data or null if an error
DataContainer* AddEventData( SharedEventList* shared_event_list, Event* event, ULONG key, ULONG type, ULONG size, PVOID data );

/// Add data to and event
/// Functionally equivalent to AddEventData but no data is specified at the moment so the container is not filled
DataContainer* ReserveEventData( SharedEventList* shared_event_list, Event* event, ULONG key, ULONG type, ULONG size );
