#include "EventRingList.h"
#include <ntstrsafe.h>
#include <strsafe.h>

#define event_ring_list_POOL_TAG 'CEV'

//#define LOCK(o) {KIRQL p_irql;			\
//	KeAcquireSpinLock(&o->lock.lock, &p_irql);	\
//	o->lock.irql = p_irql;}

EventBuffer* CreateEventBuffer( EventRingList* event_ring_list )
{
	EventBuffer* event_buffer = (EventBuffer*)ExAllocatePoolWithTag(NonPagedPool, 
					sizeof( EventBuffer ) + event_ring_list->event_buffer_length, 
					event_ring_list_POOL_TAG );
				
	if( !event_buffer )
		return NULL;

	// Setup the newly created event buffer
	RtlZeroMemory( event_buffer, sizeof( EventBuffer ) + event_ring_list->event_buffer_length );
	event_buffer->buffer_length = event_ring_list->event_buffer_length;
	event_buffer->buffer_remaining = event_ring_list->event_buffer_length;
	event_buffer->id = event_ring_list->num_event_buffers;

	return event_buffer;
}

VOID CreateEventBuffers( EventRingList* event_ring_list, unsigned int number_buffers )
{
	if( number_buffers > MAX_EVENT_BUFFERS )
		return;

	// Lock the event buffer array so it can't be changed
	if( !InterlockedBitTestAndSet( &event_ring_list->allocating_buffers, 0 ) )
	{
		unsigned int i = 0;

		DbgPrint( "EventRingList: Resizing num event buffers to %i\n", number_buffers );
		for( i = 0; i < number_buffers; i++ )
		{
			if( !event_ring_list->event_buffers[ i ] )
			{
				EventBuffer* event_buffer = CreateEventBuffer(event_ring_list);

				if( !event_buffer )
					break;

				event_ring_list->event_buffers[ i ] = event_buffer;
				event_ring_list->num_event_buffers++;	
			}
		}

		InterlockedBitTestAndReset( &event_ring_list->allocating_buffers, 0 );
	}
}

EventId CreateEvent( EventRingList* event_ring_list )
{
	Event* e = NULL;
	unsigned int i = 0;
	unsigned int event_buffer_id = -1;

	// Get an event buffer to store an event
	// Keep trying to find one, create some if they are all used up, or if there
	// is no wrong to create any more keep trying to find one until one frees up
	// NOTE: possible deadlock can occur here!
	do
	{
		for( i = 0; i < event_ring_list->num_event_buffers; i++ )
		{
			// Atomically set the buffer to be in use
			if( !InterlockedBitTestAndSet( &event_ring_list->event_buffers[ i ]->in_use, 0 ) )
			{
				event_buffer_id = i;
				break;
			}
		}

		// If there are no free event buffers
		if( event_buffer_id == -1 )
		{
			// Create some more event buffers
			CreateEventBuffers(event_ring_list, event_ring_list->num_event_buffers << 1);
		}
	} while( event_buffer_id == -1 );

	// Create an event inside the event buffer found
	e = (Event*)( event_ring_list->event_buffers[ event_buffer_id ]->buffer + event_ring_list->event_buffers[ event_buffer_id ]->buffer_used );
	event_ring_list->event_buffers[ event_buffer_id ]->buffer_used += sizeof( Event );
	event_ring_list->event_buffers[ event_buffer_id ]->buffer_remaining -= sizeof( Event );
	event_ring_list->event_buffers[ event_buffer_id ]->event = e;

	return event_buffer_id;
}

VOID RemoveEvent( EventRingList* event_ring_list, 
				  EventId event_id )
{
	// Reset the event buffers buffer and set it to not in use
	event_ring_list->event_buffers[ event_id ]->buffer_used = 0;
	event_ring_list->event_buffers[ event_id ]->buffer_remaining = event_ring_list->event_buffer_length;
	InterlockedBitTestAndReset( &event_ring_list->event_buffers[ event_id ]->in_use, 0 );
}

unsigned int AddKey( EventRingList* event_ring_list, const char* key_name )
{
	Key* key = NULL;
	unsigned int key_hash = 0;
	unsigned int key_length = 0;

	// Get the hash for the key supplied
	key_hash = GetKeyHash( event_ring_list, key_name );
	// Find the length of the key supplied
	RtlStringCbLengthA( key_name, STRSAFE_MAX_CCH * sizeof(char), &key_length );

	// Allocate enough space to hold a copy of the key
	key = (Key*)ExAllocatePoolWithTag(NonPagedPool, sizeof( Key ) + key_length + sizeof( char ), event_ring_list_POOL_TAG );

	if( !key )
		return -1;

	// Add the key to the event vectors key list
	event_ring_list->key_list_length += sizeof( Key ) + key_length + sizeof( char );

	key->next_key = NULL;
	key->key_hash = key_hash;
	key->key_length = key_length;
	RtlCopyMemory( key->key, key_name, key_length );
	key->key[key_length] = 0x00;

	// Adjust the key list
	if( !event_ring_list->head_key_list ) event_ring_list->head_key_list = key;
	if( event_ring_list->tail_key_list ) event_ring_list->tail_key_list->next_key = key;
	event_ring_list->tail_key_list = key;

	return key_hash;
}

unsigned int GetKeyHash( EventRingList* event_ring_list, const char* key_name )
{
	int i = 0;
	int c = 0;
	char* key = NULL;
	unsigned int hash = 0;

	while( c = key_name[i++] )
	{
		hash = ((hash << 5) + hash) + c;
	}

	return hash;
}

BOOLEAN AddEventData( EventRingList* event_ring_list, EventId event_id,
				      unsigned int key_hash, 
				      DataType value_type, void* value, unsigned int value_length)
{
	unsigned int data_length = 0;
	unsigned int offset = 0;
	EventData* event_data = NULL;
	EventData* tail_event_data = NULL;
	EventBuffer *event_buffer = NULL;

	event_buffer = event_ring_list->event_buffers[ event_id ];

	data_length = sizeof( EventData ) + value_length;

	// Make sure there is enough space in the event buffer to conatin the event data
	if( data_length > event_buffer->buffer_remaining )
	{
		ASSERT( FALSE );
		return FALSE;
	}

	// Create the event data inside the event buffer, reserving the required space
	event_data = (EventData*)( event_buffer->buffer + (unsigned int)event_buffer->buffer_used );
	event_buffer->buffer_used += data_length;
	event_buffer->buffer_remaining -= data_length;

	// Fill in the event data 
	event_data->next_offset = 0;
	event_data->key = key_hash;
	event_data->value_length = value_length;
	RtlCopyMemory( event_data->value, value, value_length );
	event_data->value_type = value_type;
	
	// Calculate the offset from the start of the buffer to the event data
	offset = (unsigned int)event_data - (unsigned int)event_buffer->event;

	// Add the event data to the event stored in the event buffer
	if( !event_buffer->event->head_data_offset )
		event_buffer->event->head_data_offset = offset;
	if( event_buffer->event->tail_data_offset )
	{
		tail_event_data = (EventData*)GET_EFFECTIVE_ADDRESS( (unsigned char*)event_buffer->event, (unsigned int)event_buffer->event->tail_data_offset );
		tail_event_data->next_offset = offset;
	}
	event_buffer->event->tail_data_offset = offset;

	return TRUE;
}

EventList* CreateEventList( EventRingList* event_ring_list )
{
	EventList* event_list = NULL;
	
	event_list = (EventList*)ExAllocatePoolWithTag(NonPagedPool, sizeof( EventList ) + event_ring_list->event_list_length, event_ring_list_POOL_TAG); 
	
	if( !event_list )
	{
		return NULL;
	}

	RtlZeroMemory( event_list, sizeof( EventList ) );

	// Setup the newly created event list
	KeInitializeSpinLock( &event_list->lock.lock );
	//ExInitializeFastMutex(&event_list->lock.lock);
	event_list->data_length = event_ring_list->event_list_length;
	event_list->space_remaining = event_list->data_length - event_list->space_used;
	event_list->ready = 1;

	return event_list;
}

VOID ResetEventBuffer( EventRingList* event_ring_list, EventBuffer* event_buffer )
{
	//DbgPrint("ResetEventBuffer start\n");
	// Reset the event buffer
	event_buffer->buffer_remaining = event_buffer->buffer_length;
	event_buffer->buffer_used = 0;
	event_buffer->event = NULL;

	InterlockedBitTestAndReset( &event_buffer->in_use, 0 );
	//DbgPrint("ResetEventBuffer finish\n");
}

void ResetEventList( EventRingList* event_ring_list, EventList* event_list )
{
	event_list->head_event_offset = 0;
	event_list->tail_event_offset = 0;
	event_list->next_event_list = NULL;

	event_list->ready = 1;
	event_list->space_used = 0;
	event_list->space_remaining = event_list->data_length;

	RtlZeroMemory(event_list->data, event_ring_list->event_list_length);
}

void ReleaseEventList( EventRingList* event_ring_list, EventList* event_list )
{
	KeReleaseSpinLock(&event_list->lock.lock, event_list->lock.irql);
}

EventList* AcquireReadEventList( EventRingList* event_ring_list )
{
	KIRQL irql, irql2;
	EventList* read_list = NULL;

	// Stop any new writes on the current read list
	InterlockedBitTestAndReset( &event_ring_list->read_list->ready, 0 );
	
	KeAcquireSpinLock( &event_ring_list->lock.lock, &irql);
	event_ring_list->lock.irql = irql;

		event_ring_list->read_position = (event_ring_list->read_position + 1) % event_ring_list->num_event_lists;

		// If we are reading from the current write list move the write list to the next list
		if(event_ring_list->read_position == event_ring_list->write_position)
		{
			// Move write position to the next list
			event_ring_list->write_position = (event_ring_list->write_position + 1) % event_ring_list->num_event_lists;
			event_ring_list->write_list = event_ring_list->event_lists[event_ring_list->write_position];
			ResetEventList(event_ring_list, event_ring_list->write_list);
		}

		read_list = event_ring_list->read_list;

	KeReleaseSpinLock( &event_ring_list->lock.lock, event_ring_list->lock.irql);

	// Wait for any writes on the current read list to finish
	KeAcquireSpinLock( &event_ring_list->read_list->lock.lock, &irql2);
	event_ring_list->read_list->lock.irql = irql2;

	InterlockedExchangePointer(&event_ring_list->read_list, event_ring_list->event_lists[event_ring_list->read_position]);

	return read_list;
}

EventList* AcquireNextWriteEventList( EventRingList* event_ring_list )
{
	KIRQL irql, irql2;
	EventList* write_list = NULL;

	// Stop any new writes on the current write list
	InterlockedBitTestAndReset( &event_ring_list->write_list->ready, 0 );

	KeReleaseSpinLock( &event_ring_list->write_list->lock.lock, event_ring_list->write_list->lock.irql);

	KeAcquireSpinLock( &event_ring_list->lock.lock, &irql);
	event_ring_list->lock.irql = irql;

		// Move onto the next write list
		event_ring_list->write_position = (event_ring_list->write_position + 1) % event_ring_list->num_event_lists;

		if( event_ring_list->write_position == event_ring_list->read_position )
		{
			event_ring_list->read_position = (event_ring_list->read_position + 1) % event_ring_list->num_event_lists;
			event_ring_list->read_list = event_ring_list->event_lists[event_ring_list->read_position];
			DbgPrint("EventList-AcquireWriteEventList: EVENTS LOST: %08x - %i\n", event_ring_list->read_list, event_ring_list->read_position);
		}

		write_list = event_ring_list->event_lists[event_ring_list->write_position];

	KeReleaseSpinLock( &event_ring_list->lock.lock, event_ring_list->lock.irql);

	KeAcquireSpinLock( &write_list->lock.lock, &irql2);
	write_list->lock.irql = event_ring_list->write_list->lock.irql;

	ResetEventList(event_ring_list, write_list);
	
	InterlockedExchangePointer(&event_ring_list->write_list, write_list);

	return write_list;
}

EventList* AcquireWriteEventList( EventRingList* event_ring_list, unsigned int write_size )
{
	KIRQL irql;

	// Get an event list to write to
	do
	{
		EventList* event_list = event_ring_list->write_list;

		// Lock the event list so nothing can be added to it
		KeAcquireSpinLock( &event_list->lock.lock, &irql);
		event_list->lock.irql = irql;

		// Make sure the current write list is ready to be written to. 
		// If it isn't it is probably changing to another list so skip
		// it and keep trying until the write list is setup
		if( event_list->ready )
		{	
			if( write_size > event_list->space_remaining )
			{
				// Not enough room in current list so acquire the next and set the
				// next event list to write to. This will guarantee that events
				// will be inserted in the order that they occur
				event_list = AcquireNextWriteEventList(event_ring_list);
			}
			return event_list;
		}
		
		KeReleaseSpinLock( &event_list->lock.lock, event_list->lock.irql);
	}
	while(TRUE);	// Loop until we have acquired a list to write to
}

Event* AddEvent( EventRingList* event_ring_list, EventId event_id )
{	
	
	EventList* event_list = NULL;
	EventBuffer *event_buffer = NULL;
	PVOID event_list_data_position = NULL;
	unsigned int offset = 0;

	event_buffer = event_ring_list->event_buffers[ event_id ];

	// Get an event list to write to
	event_list = AcquireWriteEventList( event_ring_list, event_buffer->buffer_used );

	// Copy the event into the event list
	event_list_data_position = event_list->data + event_list->space_used;
	RtlCopyBytes( event_list_data_position, event_buffer->buffer, event_buffer->buffer_used );
	event_list->space_used += event_buffer->buffer_used;
	event_list->space_remaining -= event_buffer->buffer_used;

	offset = (unsigned int)event_list_data_position - (unsigned int)event_list;

	// Insert the event into the event linked list
	if( !event_list->head_event_offset )
		event_list->head_event_offset = offset;
	if( event_list->tail_event_offset )
	{
		Event* tail_event = (Event*)GET_EFFECTIVE_ADDRESS( (unsigned char*)event_list, (unsigned int)event_list->tail_event_offset );
		tail_event->next_offset = offset;
	}
	event_list->tail_event_offset = offset;

	ResetEventBuffer( event_ring_list, event_buffer );

	ReleaseEventList( event_ring_list, event_list );

	return (Event*)event_list_data_position;
}

void InitialiseEventRingList( EventRingList* event_ring_list,
							unsigned int reserve_length,
						    unsigned int initial_num_buffers, unsigned int buffer_length,
						    unsigned int max_keys, unsigned int max_key_size )
{
	unsigned int i = 0;
	BOOLEAN error = FALSE;

	RtlZeroMemory( event_ring_list, sizeof( EventRingList ) );

	event_ring_list->num_event_buffers = initial_num_buffers;
	event_ring_list->event_buffer_length = buffer_length;
	event_ring_list->event_list_length = reserve_length;
	event_ring_list->num_event_lists = 4;

	KeInitializeSpinLock( &event_ring_list->lock.lock );

	for( i = 0; i < event_ring_list->num_event_buffers; i++ )
	{
		event_ring_list->event_buffers[i] = CreateEventBuffer(event_ring_list);
		if(!event_ring_list->event_buffers[i]) { error = TRUE; break; }
	}

	for(i = 0; i < event_ring_list->num_event_lists; i++)
	{
		event_ring_list->event_lists[i] = CreateEventList(event_ring_list);
		if( !event_ring_list->event_lists[i] )	{ error = TRUE; break; }
		DbgPrint("Initialised Event List: %08x - %i\n", event_ring_list->event_lists[i], i );
	}

	event_ring_list->read_position = 0;
	event_ring_list->write_position = 1;

	event_ring_list->read_list = event_ring_list->event_lists[event_ring_list->read_position];
	event_ring_list->write_list = event_ring_list->event_lists[event_ring_list->write_position];

	if( error )
	{
		DeleteEventRingList(event_ring_list);
	}
}

void DeleteEventRingList( EventRingList* event_ring_list )
{
	Key* key = NULL;
	unsigned int i = 0;	
	
	// Free the event buffers
	for(i = 0; i < event_ring_list->num_event_buffers; i++)
	{
		if(event_ring_list->event_buffers[i])
		{
			ExFreePoolWithTag(event_ring_list->event_buffers[i], event_ring_list_POOL_TAG);
		}
	}

	// Free the event lists
	for(i = 0; i < event_ring_list->num_event_lists; i++)
	{
		if(event_ring_list->event_lists[i])
		{
			ExFreePoolWithTag( event_ring_list->event_lists[i], event_ring_list_POOL_TAG);
		}
	}

	// Delete all the keys stored in the vector
	key = event_ring_list->head_key_list;
	while( key )
	{
		Key* next_key = key->next_key;
		ExFreePoolWithTag( key, event_ring_list_POOL_TAG );
		key = next_key;
	}

	RtlZeroMemory( event_ring_list, sizeof( EventRingList ) );
}

PVOID ReserveEventDataSpace( EventRingList* event_ring_list, 
							 EventId event_id, 
							 unsigned int key_hash, DataType value_type, unsigned int value_length )
{
	unsigned int data_length = 0;
	unsigned int offset = 0;
	EventData* event_data = NULL;
	EventData* tail_event_data = NULL;
	EventBuffer *event_buffer = NULL;

	event_buffer = event_ring_list->event_buffers[ event_id ];

	data_length = sizeof( EventData ) + value_length;

	if( data_length > event_buffer->buffer_remaining )
	{
		ASSERT( FALSE );
		return NULL;
	}

	event_data = (EventData*)( event_buffer->buffer + (unsigned int)event_buffer->buffer_used );
	event_buffer->buffer_used += data_length;
	event_buffer->buffer_remaining -= data_length;

	event_data->next_offset = 0;
	event_data->key = key_hash;
	event_data->value_length = value_length;
	event_data->value_type = value_type;

	offset = (unsigned int)event_data - (unsigned int)event_buffer->event;

	if( !event_buffer->event->head_data_offset )
	{
		event_buffer->event->head_data_offset = offset;
	}

	if( event_buffer->event->tail_data_offset )
	{
		tail_event_data = (EventData*)GET_EFFECTIVE_ADDRESS( (unsigned char*)event_buffer->event, (unsigned int)event_buffer->event->tail_data_offset );
		tail_event_data->next_offset = offset;
	}
	event_buffer->event->tail_data_offset = offset;

	return (PVOID)event_data->value;
}

NTSTATUS HandleEventRingListIoctl( EventRingList* event_ring_list, 
								 IN PDEVICE_OBJECT device_object,
								 PIRP irp,
								 PIO_STACK_LOCATION io_stack_location,
								 PULONG bytes_written )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

	*bytes_written = 0;
	
    switch( io_stack_location->Parameters.DeviceIoControl.IoControlCode )
    {
	case GET_event_ring_list_LIST_SIZE:
		//DbgPrint( "EventRingList: GET_event_ring_list_LIST_SIZE\n");
		status = HandleGetEventRingListListSizeIoctl( event_ring_list, device_object, irp, io_stack_location, bytes_written );
		break;
	case GET_event_ring_list_KEYS_SIZE:
		//DbgPrint( "EventRingList: GET_event_ring_list_KEYS_SIZE\n");
		status = HandleGetEventRingListKeysSizeIoctl( event_ring_list, device_object, irp, io_stack_location, bytes_written );
		break;
	case GET_event_ring_list_KEYS:
		//DbgPrint( "EventRingList: GET_event_ring_list_KEYS\n");
		status = HandleGetEventRingListKeysIoctl( event_ring_list, device_object, irp, io_stack_location, bytes_written );
		break;
	case GET_event_ring_list_LIST:
		//DbgPrint( "EventRingList: GET_event_ring_list_LIST\n");
		status = HandleGetEventRingListListIoctl( event_ring_list, device_object, irp, io_stack_location, bytes_written );
		break;
	default:
		//DbgPrint( "EventRingList: UNKNOWN IOCTL %08x\n", io_stack_location->Parameters.DeviceIoControl.IoControlCode);
		break;
    }

	return status;
}

NTSTATUS HandleGetEventRingListListSizeIoctl( EventRingList* event_ring_list, 
											IN PDEVICE_OBJECT device_object, 
											PIRP irp,
											PIO_STACK_LOCATION io_stack_location,
											PULONG bytes_written )
{
	PCHAR output_buffer = NULL;
	ULONG output_buffer_length = 0;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

	try
	{
		// Make sure we can write to the buffer supplied
		ProbeForWrite( irp->UserBuffer, io_stack_location->Parameters.DeviceIoControl.OutputBufferLength, __alignof( unsigned long ) );

		output_buffer = irp->UserBuffer;
		output_buffer_length = io_stack_location->Parameters.DeviceIoControl.OutputBufferLength;

		if( !output_buffer )
		{
			return status;
		}

		// Make sure there is enough room in the output buffer
		if( output_buffer_length < sizeof( unsigned long ) )
		{
			return STATUS_BUFFER_TOO_SMALL;
		}

		RtlCopyMemory( output_buffer, &event_ring_list->event_list_length, sizeof( unsigned long ) );
		
		*bytes_written = sizeof( unsigned long );
	
		status = STATUS_SUCCESS;
	} 
	except( EXCEPTION_EXECUTE_HANDLER )
	{
		status = GetExceptionCode();    
		//DbgPrint("Capture-EventRingList: Exception - GET_event_ring_list_LIST_SIZE - %08x\n", status);		 
	}

	return status;
}

NTSTATUS HandleGetEventRingListKeysSizeIoctl( EventRingList* event_ring_list, 
											IN PDEVICE_OBJECT device_object, 
											PIRP irp,
											PIO_STACK_LOCATION io_stack_location,
											PULONG bytes_written )
{
	PCHAR output_buffer = NULL;
	ULONG output_buffer_length = 0;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

	__try
	{
		// Make sure we can write to the buffer supplied
		ProbeForWrite( irp->UserBuffer, io_stack_location->Parameters.DeviceIoControl.OutputBufferLength, __alignof ( unsigned long ) );

		output_buffer = irp->UserBuffer;
		output_buffer_length = io_stack_location->Parameters.DeviceIoControl.OutputBufferLength;

		if( !output_buffer )
		{
			return status;
		}

		// Make sure there is enough room in the output buffer
		if( output_buffer_length < sizeof( unsigned long ) )
		{
			return STATUS_BUFFER_TOO_SMALL;
		}

		RtlCopyMemory( output_buffer, &event_ring_list->key_list_length, sizeof( unsigned long ) );
		
		*bytes_written = sizeof( unsigned long );
	
		status = STATUS_SUCCESS;
	} 
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		status = GetExceptionCode(); 
		//DbgPrint("Capture-EventRingList: Exception - GET_event_ring_list_KEYS_SIZE - %08x\n", status);		 
	}

	return status;
}

NTSTATUS HandleGetEventRingListKeysIoctl( EventRingList* event_ring_list, 
										 IN PDEVICE_OBJECT device_object, 
									     PIRP irp,
									     PIO_STACK_LOCATION io_stack_location,
									     PULONG bytes_written )
{
	PCHAR output_buffer = NULL;
	ULONG output_buffer_length = 0;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

	output_buffer = irp->UserBuffer;
	output_buffer_length = io_stack_location->Parameters.DeviceIoControl.OutputBufferLength;

	if( !output_buffer )
	{
		return status;
	}

	// Make sure there is enough room in the output buffer
	if( output_buffer_length < event_ring_list->key_list_length )
	{
		return STATUS_BUFFER_TOO_SMALL;
	}

	try
	{
		Key* key = NULL;
		unsigned int offset = 0;
		// Make sure we can write to the buffer supplied
		ProbeForWrite( output_buffer, output_buffer_length, sizeof( UCHAR ) );

		// Copy all the keys into the output buffer. We already know that the output buffer is
		// bug enough to store the array as the key list is created in the drivers main function
		key = event_ring_list->head_key_list;
		
		while( key )
		{
			RtlCopyMemory( output_buffer + offset, key, sizeof( Key ) + key->key_length + sizeof( char ));
			offset += sizeof( Key ) + key->key_length + sizeof( char );
			key = key->next_key;
		}
		
		*bytes_written = offset;
	
		status = STATUS_SUCCESS;
	} 
	except( EXCEPTION_EXECUTE_HANDLER )
	{
		status = GetExceptionCode();    
		//DbgPrint("Capture-EventRingList: Exception - GET_event_ring_list_KEYS - %08x\n", status);		 
	}

	return status;
}

NTSTATUS HandleGetEventRingListListIoctl( EventRingList* event_ring_list, 
										IN PDEVICE_OBJECT device_object, 
									    PIRP irp,
									    PIO_STACK_LOCATION io_stack_location,
									    PULONG bytes_written )
{
	PCHAR output_buffer = NULL;
	ULONG output_buffer_length = 0;
	EventList* event_list = NULL;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

	output_buffer = irp->UserBuffer;
	output_buffer_length = io_stack_location->Parameters.DeviceIoControl.OutputBufferLength;

	if( !output_buffer )
	{
		//DbgPrint("CaptureRegistryMonitor: No Buffer\n");
		return status;
	}

	// Make sure the output buffer at least matches the default event list size
	if( output_buffer_length < event_ring_list->event_list_length )
	{
		DbgPrint("EventRingList: Buffer to small %i\n", output_buffer_length);
		return STATUS_BUFFER_TOO_SMALL;
	}

	try
	{
		// Make sure we can write to the buffer supplied
		ProbeForWrite( output_buffer, output_buffer_length, sizeof( UCHAR ) );
		
		// Get and event list
		event_list = AcquireReadEventList( event_ring_list );

		//DbgPrint( "CaptureRegistryMonitor: Got event list %i\n", event_list->space_used );
		
		RtlCopyMemory( output_buffer, event_list, event_list->space_used + sizeof( EventList ) );
		
		*bytes_written = event_list->space_used + sizeof( EventList );
		
		ReleaseEventList( event_ring_list , event_list );
		
		status = STATUS_SUCCESS;
	} 
	except( EXCEPTION_EXECUTE_HANDLER )
	{
		status = GetExceptionCode();    
		//DbgPrint("Capture-EventRingList: Exception - GET_event_ring_list_LIST - %08x\n", status);		 
	}

	return status;
}