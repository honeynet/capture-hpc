/*
 *	PROJECT: Capture
 *	FILE: CaptureConnectionMonitor.h
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
#include "ntifs.h"
#include "ntddk.h"
#include "tdikrnl.h"

#include "HashMap.h"

typedef unsigned int UINT;

#define CONNECTION_POOL_TAG			'cCM'

typedef enum _TcpState TcpState;

enum _TcpState
{
	TCP_STATE_CLOSED,
	TCP_STATE_LISTEN,
	TCP_STATE_SYN_SENT,
	TCP_STATE_SYN_RCVD,
	TCP_STATE_ESTABLISHED,
	TCP_STATE_FIN_WAIT_1,
	TCP_STATE_FIN_WAIT_2,
	TCP_STATE_CLOSING,
	TCP_STATE_TIME_WAIT,
	TCP_STATE_CLOSE_WAIT,
	TCP_STATE_LAST_ACK
};

typedef struct _ConnectionManager ConnectionManager;
typedef struct _ConnectionContext ConnectionContext;
typedef struct _Connection	Connection;
typedef struct _ClientEventConnectContext ClientEventConnectContext;
typedef struct _CompletionRoutine CompletionRoutine;
typedef struct _CompletionRoutineContext CompletionRoutineContext;
typedef struct _AddressWorkItem AddressWorkItem;

struct _CompletionRoutine
{
	PIO_COMPLETION_ROUTINE callback;
	PVOID context;
	UCHAR control;
};

struct _CompletionRoutineContext
{
	CompletionRoutine completion_routine;
	PIRP original_irp;
};

struct _ClientEventConnectContext
{
	Connection*			connection;
	CompletionRoutine	previous_callback;
};

struct _AddressWorkItem
{
	WORK_QUEUE_ITEM	work_item;

	PDEVICE_OBJECT	device_object;
	Connection*		connection;
};

struct _Connection
{
	Connection*		next;
	Connection*		prev;

	UINT			process_id;
	PFILE_OBJECT	local_node;
	PFILE_OBJECT	connection;
	PFILE_OBJECT	remote_node;
	PTA_ADDRESS		local_address;
	PTA_ADDRESS		remote_address;
	TcpState		state;
};

struct _ConnectionContext
{
	ConnectionManager*	connection_manager;
	PVOID				original_callback;
	PVOID				original_context;
	PFILE_OBJECT		file_object;
};

struct _ConnectionManager
{
	PDEVICE_OBJECT	next_device;
	HashMap			connection_hash_map;
	HashMap			callback_hash_map;

	BOOLEAN			ready;

	Connection*		connection_head;
	Connection*		connection_tail;

	KSPIN_LOCK		lock;
};

const CHAR* EventCallbackNames[] =
{
	"TDI_EVENT_CONNECT",
	"TDI_EVENT_DISCONNECT",
	"TDI_EVENT_ERROR",
	"TDI_EVENT_RECEIVE",
	"TDI_EVENT_RECEIVE_DATAGRAM",
	"TDI_EVENT_RECEIVE_EXPEDITED",
	"TDI_EVENT_SEND_POSSIBLE",
	"TDI_EVENT_CHAINED_RECEIVE",
	"TDI_EVENT_CHAINED_RECEIVE_DATAGRAM",
	"TDI_EVENT_CHAINED_RECEIVE_EXPEDITED",
	"TDI_EVENT_ERROR_EX"
};

const CHAR* TdiIrpNames[] =
{
	"",
	"TDI_ASSOCIATE_ADDRESS",
	"TDI_DISASSOCIATE_ADDRESS",
	"TDI_CONNECT",
	"TDI_LISTEN",
	"TDI_ACCEPT",
	"TDI_DISCONNECT",
	"TDI_SEND",
	"TDI_RECEIVE",
	"TDI_SEND_DATAGRAM",
	"TDI_RECEIVE_DATAGRAM",
	"TDI_SET_EVENT_HANDLER",
	"TDI_QUERY_INFORMATION",
	"TDI_SET_INFORMATION",
	"TDI_ACTION",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"TDI_DIRECT_SEND",
	"TDI_DIRECT_SEND_DATAGRAM",
	"TDI_DIRECT_ACCEPT"
};

// Client TDI callbacks
NTSTATUS ClientEventConnect(
    IN PVOID				event_context,
    IN LONG					remote_address_length,
    IN PVOID				remote_address,
    IN LONG					user_data_length,
    IN PVOID				user_data,
    IN LONG					options_length,
    IN PVOID				options,
    OUT CONNECTION_CONTEXT	*connection_context,
    OUT PIRP				*accept_irp
    );

NTSTATUS ClientEventDisconnect(
    IN PVOID				event_context,
    IN CONNECTION_CONTEXT	connection_context,
    IN LONG					disconnect_data_length,
    IN PVOID				disconnect_data,
    IN LONG					disconnect_information_length,
    IN PVOID				disconnect_information,
    IN ULONG				disconnect_flags
    );

NTSTATUS ClientEventReceive(
    IN PVOID				event_context,
    IN CONNECTION_CONTEXT	connection_context,
    IN ULONG				receive_flasg,
    IN ULONG				bytes_indicated,
    IN ULONG				bytes_available,
    OUT ULONG				*bytes_taken,
    IN PVOID				tsdu,
    OUT PIRP				*io_request_packet
    );

const PVOID ConnectionCallbacks[] =
{
	ClientEventConnect,			// TDI_EVENT_CONNECT
	ClientEventDisconnect,		// TDI_EVENT_DISCONNECT
	NULL,						// TDI_EVENT_ERROR
	ClientEventReceive,			// TDI_EVENT_RECEIVE
	NULL,						// TDI_EVENT_RECEIVE_DATAGRAM 
	NULL,						// TDI_EVENT_RECEIVE_EXPEDITED
	NULL,						// TDI_EVENT_SEND_POSSIBLE
	NULL,						// TDI_EVENT_CHAINED_RECEIVE
	NULL,						// TDI_EVENT_CHAINED_RECEIVE_DATAGRAM
	NULL,						// TDI_EVENT_CHAINED_RECEIVE_EXPEDITED
	NULL						// TDI_EVENT_ERROR_EX
};

// TDI IRP Completion Handlers
NTSTATUS	TdiCompletionHandler(PDEVICE_OBJECT device_object, PIRP irp, PVOID context);
NTSTATUS	TdiNewIrpCompletionHandler(PDEVICE_OBJECT device_object, PIRP irp, PVOID context);
NTSTATUS	TdiCallCompletionRoutine(CompletionRoutine* completion_routine, PDEVICE_OBJECT device_object, PIRP irp);

// TDI IRP handlers
NTSTATUS	TdiDefaultHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiAssociateAddressHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiDisassociateAddressHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiConnectHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiListenHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiAcceptHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiDisconnectHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiSendHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiReceiveHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiSendDatagramHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiReceiveDatagramHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiSetEventHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiQueryInformationHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiSetInformationHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiActionHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );

typedef long (*TdiIrpHandler)( ConnectionManager*, PIRP, PIO_STACK_LOCATION, CompletionRoutine* completion_routine );

const PVOID tdi_irp_handlers[] =
{
	TdiDefaultHandler,
	TdiAssociateAddressHandler,
	TdiDisassociateAddressHandler,
	TdiConnectHandler,
	TdiListenHandler,
	TdiAcceptHandler,
	TdiDisconnectHandler,
	TdiSendHandler,
	TdiReceiveHandler,
	TdiSendDatagramHandler,
	TdiReceiveDatagramHandler,
	TdiSetEventHandler,
	TdiQueryInformationHandler,
	TdiSetInformationHandler,
	TdiActionHandler
};

const int num_tdi_irp_handlers = sizeof(tdi_irp_handlers);

// sprintf kernel driver equivalent
void VPrintf( char* buffer, int buffer_size, const char* format, ... );



// Converts a TA_ADDRESS to a string
void AddressToString( PTA_ADDRESS address_info, char* buffer, int buffer_size );
void DbgPrintConnection( Connection* connection );

Connection* CreateConnection( ConnectionManager* connection_manager, int process_id, PFILE_OBJECT local, PFILE_OBJECT remote );
//Connection* AllocateConnection( ConnectionManager* connection_manager );
void FreeConnection( ConnectionManager* connection_manager, Connection* connection );

void CopyAddress( PTA_ADDRESS* destination, PTA_ADDRESS source );

void GetLocalAddressWorkItem( PVOID user_data );
void GetLocalAddress( PDEVICE_OBJECT device_object, Connection* connection );
NTSTATUS GetLocalAddressComplete( PDEVICE_OBJECT device_object, PIRP irp, PVOID context );

// Kernel driver stuff
NTSTATUS DriverEntry( IN PDRIVER_OBJECT driver_object, IN PUNICODE_STRING registry_path );
VOID UnloadDriver( IN PDRIVER_OBJECT driver_object );
