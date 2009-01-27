#define NTDDI_WINXPSP2                      0x05010200
#define OSVERSION_MASK      0xFFFF0000
#define SPVERSION_MASK      0x0000FF00
#define SUBVERSION_MASK     0x000000FF


//
// macros to extract various version fields from the NTDDI version
//
#define OSVER(Version)  ((Version) & OSVERSION_MASK)
#define SPVER(Version)  (((Version) & SPVERSION_MASK) >> 8)
#define SUBVER(Version) (((Version) & SUBVERSION_MASK) )
//#define NTDDI_VERSION   NTDDI_WINXPSP2

#include <ntifs.h>
#include <wdm.h>

#define MAX_NUM_EVENT_LISTS 8
#define MAX_EVENT_BUFFERS 256
#define MAX_EVENT_LISTS 16

#define GET_EFFECTIVE_ADDRESS(b, o) ( o ? b + o : 0 )
#define TO_OFFSET_ADDRESS(b, a) ( a - b )

#define GET_event_ring_list_LIST_SIZE		CTL_CODE( 0x00000022, 0x810, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA )
#define GET_event_ring_list_KEYS_SIZE		CTL_CODE( 0x00000022, 0x811, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA )
#define GET_event_ring_list_KEYS			CTL_CODE( 0x00000022, 0x812, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA )
#define GET_event_ring_list_LIST			CTL_CODE( 0x00000022, 0x813, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA )

typedef unsigned int EventId;

//typedef struct _DataType DataType;
typedef struct _Lock Lock;
typedef struct _Key Key;
typedef struct _EventData EventData;
typedef struct _Event Event;
typedef struct _EventBuffer EventBuffer;
typedef struct _EventList EventList;
typedef struct _EventRingList EventRingList;

typedef enum _DataType
{
	int_t,
	uint_t,
	float_t,
	void_t,
	long_t,
	ulong_t,
	pwchar_t,
	unicode_string_t
} DataType;

struct _Lock
{
	KIRQL irql;
	//FAST_MUTEX lock;
	KSPIN_LOCK lock;
};

struct _Key
{
	Key* next_key;
	unsigned int key_hash;
	unsigned int key_length;
	char key[];
};

struct _EventData
{
	unsigned int next_offset;
	unsigned int key;
	unsigned int value_type;
	unsigned int value_length;
	unsigned char value[];
};

struct _Event
{
	unsigned int next_offset;

	unsigned int head_data_offset;
	unsigned int tail_data_offset;
};

struct _EventBuffer
{
	Event* event;
	unsigned int id;
	long volatile in_use;

	unsigned int buffer_used;
	unsigned int buffer_remaining;
	unsigned int buffer_length;

	unsigned char buffer[];
};

struct _EventList
{
	EventList* next_event_list;

	unsigned int head_event_offset;
	unsigned int tail_event_offset;

	Lock lock;
	long volatile ready;

	unsigned int space_used;
	unsigned int space_remaining;

	unsigned int data_length;
	unsigned char data[];
};

struct _EventRingList
{
	unsigned int num_event_lists;
	EventList* event_lists[MAX_EVENT_LISTS];

	EventList* read_list;
	EventList* write_list;

	unsigned int read_position;
	unsigned int write_position;

	Lock lock;

	unsigned long event_list_length;
	
	Key* head_key_list;
	Key* tail_key_list;
	unsigned long key_list_length;

	long volatile allocating_buffers;

	unsigned long event_buffer_length;
	unsigned int num_event_buffers;
	EventBuffer* event_buffers[MAX_EVENT_BUFFERS];
};

unsigned int GetKeyHash( EventRingList* event_ring_list, const char* key_name );
unsigned int AddKey( EventRingList* event_ring_list, const char* key_name );


EventList* AcquireReadEventList( EventRingList* event_ring_list );
EventList* AcquireWriteEventList( EventRingList* event_ring_list, unsigned int write_size );
void ReleaseEventList( EventRingList* event_ring_list, EventList* event_list );
void ResetEventList( EventRingList* event_ring_list, EventList* event_list );

void CreateEventBuffers( EventRingList* event_ring_list, unsigned int number_buffers );

void InitialiseEventRingList( EventRingList* event_ring_list,
							unsigned int reserve_length,
						    unsigned int initial_num_buffers, unsigned int buffer_length,
						    unsigned int max_keys, unsigned int max_key_size );

void DeleteEventRingList( EventRingList* event_ring_list );

Event* AddEvent( EventRingList* event_ring_list, 
				 EventId event_id );
VOID RemoveEvent( EventRingList* event_ring_list, 
				  EventId event_id );

EventId CreateEvent( EventRingList* event_ring_list );

BOOLEAN AddEventData( EventRingList* event_ring_list, 
					  EventId event_id,
				      unsigned int key_hash, 
				      DataType value_type, void* value, unsigned int value_length );

PVOID ReserveEventDataSpace( EventRingList* event_ring_list, 
							 EventId event_id, 
							 unsigned int key_hash, DataType value_type, unsigned int value_length );

void SetupWriteEventList( EventRingList* event_ring_list, EventList* event_list );
void ResetEventList( EventRingList* event_ring_list, EventList* event_list );
void ReleaseEventList( EventRingList* event_ring_list,
					   EventList* event_list );
void AddEventList( EventRingList* event_ring_list, EventList* event_list );
EventList* CreateEventList( EventRingList* event_ring_list );

NTSTATUS HandleEventRingListIoctl( EventRingList* event_ring_list, 
								 IN PDEVICE_OBJECT device_object,
								 PIRP irp,
								 PIO_STACK_LOCATION io_stack_location,
								 PULONG bytes_written );

NTSTATUS HandleGetEventRingListListSizeIoctl( EventRingList* event_ring_list, 
											IN PDEVICE_OBJECT device_object, 
											PIRP irp,
											PIO_STACK_LOCATION io_stack_location,
											PULONG bytes_written );

NTSTATUS HandleGetEventRingListKeysSizeIoctl( EventRingList* event_ring_list, 
											IN PDEVICE_OBJECT device_object, 
											PIRP irp,
											PIO_STACK_LOCATION io_stack_location,
											PULONG bytes_written );

NTSTATUS HandleGetEventRingListKeysIoctl( EventRingList* event_ring_list, 
										 IN PDEVICE_OBJECT device_object, 
									     PIRP irp,
									     PIO_STACK_LOCATION io_stack_location,
									     PULONG bytes_written );

NTSTATUS HandleGetEventRingListListIoctl( EventRingList* event_ring_list, 
										IN PDEVICE_OBJECT device_object, 
									    PIRP irp,
									    PIO_STACK_LOCATION io_stack_location,
									    PULONG bytes_written );