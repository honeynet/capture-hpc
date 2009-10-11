/*
*	PROJECT: Capture
*	FILE: CaptureTdiEventHandlers.c
*	AUTHORS:	Ramon Steenson (rsteenson@gmail.com)
*				Van Lam Le (vanlamle@gmail.com)
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
#include "CaptureConnectionMonitor.h"
#include "Connection.h"
#include "ConnectionManager.h"

#include "CaptureTdiEventHandlers.h"

#include "EventHandlerManager.h"

const CHAR* TdiDisconnectFlagNames[];

NTSTATUS TdiConnectEventHandlerComplete( PDEVICE_OBJECT device_object, PIRP irp, PVOID context )
{
	Connection* connection = (Connection*)context;
	ConnectionManager* connection_manager = NULL;

	connection_manager = (ConnectionManager*)device_object->DeviceExtension;

	if(connection)
	{
		if( irp->IoStatus.Status == STATUS_SUCCESS )
		{
			SetConnectionState(connection_manager, connection, TCP_STATE_ESTABLISHED);
		}
		else
		{
			SetConnectionState(connection_manager, connection, TCP_STATE_FIN_WAIT_1);
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS TdiConnectEventHandler(
								IN PVOID  tdiEventContext,
								IN LONG  remoteAddressLength,
								IN PVOID  remoteAddress,
								IN LONG  userDataLength,
								IN PVOID  userData,
								IN LONG  optionsLength,
								IN PVOID  options,
								OUT CONNECTION_CONTEXT  *connectionContext,
								OUT PIRP  *acceptIrp
								)
{
	NTSTATUS status;
	Connection* connection = NULL;
	EventHandler* event_handler = (EventHandler*)tdiEventContext;

	DbgPrint("TdiConnectEventHandler\t\t\tNode = %08x \n", event_handler->node);

	status = ((PTDI_IND_CONNECT)event_handler->original_handler.EventHandler)(
		event_handler->original_handler.EventContext,
		remoteAddressLength,
		remoteAddress,
		userDataLength,
		userData,
		optionsLength,
		options,
		connectionContext,
		acceptIrp);

	// Received SYN, Sent SYN,ACK
	if( status == STATUS_MORE_PROCESSING_REQUIRED )
	{
		PTA_ADDRESS remote_address = NULL;
		PIO_STACK_LOCATION irp_stack = NULL;
		CompletionRoutineContext* context = NULL;

		irp_stack = IoGetCurrentIrpStackLocation( *acceptIrp );

		connection = GetConnection( event_handler->connection_manager, irp_stack->FileObject );

		if(connection == NULL)
		{
			DbgPrint("Can't find connection for node %08x\n",irp_stack->FileObject);
			return status;
		}
		else 
		{
			DbgPrint("Connection \t\tLocal node:%08x \t\tRemote node:%08x\n",connection->local_node, connection->remote_node);
			DbgPrint("Context file object:%08x\n", irp_stack->FileObject);
		}
		GetLocalAddress(connection, event_handler->connection_manager->next_device);

		remote_address = ((TRANSPORT_ADDRESS *)remoteAddress)->Address;
		
		SetRemoteNodeAddress(connection, remote_address);
		
		SetConnectionState( event_handler->connection_manager, connection, TCP_STATE_SYN_RCVD );

		// NOTE: acceptIrp will bypass the driver stack so we receive no TDI_ACCEPT irp
		// so we set a completion routine up here to be notified on the status
		// of the accept
		context = (CompletionRoutineContext*)ExAllocatePoolWithTag(NonPagedPool, sizeof(CompletionRoutineContext), CONNECTION_POOL_TAG);

		if(context)
		{
			context->completion_routine.callback = TdiConnectEventHandlerComplete;
			context->completion_routine.context = connection;
			context->completion_routine.control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;

			// Set our completion routine
			IoCopyCurrentIrpStackLocationToNext( *acceptIrp );
			IoSetCompletionRoutine( *acceptIrp, TdiCompletionHandler, context, TRUE, TRUE, TRUE );

			// Make our completion routine the first to be called
			IoSetNextIrpStackLocation(*acceptIrp);
		}
	}
	else
	{
		// Connection refused

		//connection = GetConnection( event_handler->connection_manager, event_handler->node );

		//if(connection == NULL)
		//	return status;

		//SetConnectionState( event_handler->connection_manager, connection, TCP_STATE_CLOSED );
	}

	return status;
}

NTSTATUS TdiDisconnectEventHandler(
								   IN PVOID  tdiEventContext,
								   IN CONNECTION_CONTEXT  connectionContext,
								   IN LONG  disconnectDataLength,
								   IN PVOID  disconnectData,
								   IN LONG  disconnectInformationLength,
								   IN PVOID  disconnectInformation,
								   IN ULONG  disconnectFlags
								   )
{
	NTSTATUS status;
	Connection* connection = NULL;
	EventHandler* event_handler = (EventHandler*)tdiEventContext;

	connection = GetConnection( event_handler->connection_manager, event_handler->node );

	status = ((PTDI_IND_DISCONNECT)event_handler->original_handler.EventHandler)(
		event_handler->original_handler.EventContext,
		connectionContext,
		disconnectDataLength,
		disconnectData,
		disconnectInformationLength,
		disconnectInformation,
		disconnectFlags);
	if (connection==NULL)
	{
		return status;
	}
	else
	{
		DbgPrint("TdiDisconnectEventHandler\t\t\tNode = %08x %s connection=%08x\t\tLocal node=%08x\t\tRemote Node=%08x\n", event_handler->node, TdiDisconnectFlagNames[disconnectFlags], connection,connection->local_node, connection->remote_node);
	}
	
	if(disconnectFlags == TDI_DISCONNECT_RELEASE)
	{
		// Received a FIN
		if(connection->state == TCP_STATE_FIN_WAIT_1)
		{
			SetConnectionState( event_handler->connection_manager, connection, TCP_STATE_TIME_WAIT);
		}
		else if(connection->state == TCP_STATE_ESTABLISHED)
		{
			SetConnectionState( event_handler->connection_manager, connection, TCP_STATE_CLOSE_WAIT);
		}
	}
	else if(disconnectFlags == TDI_DISCONNECT_ABORT)
	{
		// Recevied an RST
		SetConnectionState( event_handler->connection_manager, connection, TCP_STATE_CLOSED);
	}

	// Received a FIN packet	
	//{
	//	Connection* connection = NULL;

	//	connection = GetConnection( event_handler->connection_manager, event_handler->node );



	//	if(connection->state == TCP_STATE_ESTABLISHED)
	//	{
	//		SetConnectionState( event_handler->connection_manager, connection, TCP_STATE_CLOSE_WAIT);
	//	}
	//	else if(connection->state == TCP_STATE_FIN_WAIT_1)
	//	{
	//		SetConnectionState( event_handler->connection_manager, connection, TCP_STATE_CLOSING);
	//	}
	//	else if(connection->state == TCP_STATE_FIN_WAIT_2)
	//	{
	//		SetConnectionState( event_handler->connection_manager, connection, TCP_STATE_TIME_WAIT);
	//	}
	//	else
	//	{
	//		// Not sure? TCP diagram says we shouldnt be at this state. Prob bug somewhere
	//		DbgBreakPoint();
	//	}
	//}


	return status;
}

NTSTATUS TdiErrorEventHandler(
							  IN PVOID  TdiEventContext,
							  IN NTSTATUS  Status
							  )
{
	NTSTATUS status;

	EventHandler* event_handler = (EventHandler*)TdiEventContext;

	DbgPrint("TdiErrorEventHandler\t\t\tNode = %08x \n", event_handler->node);

	status = ((PTDI_IND_ERROR)event_handler->original_handler.EventHandler)(
		event_handler->original_handler.EventContext,
		Status);

	return status;
}

NTSTATUS TdiReceiveEventHandler(
								IN PVOID  tdiEventContext,
								IN CONNECTION_CONTEXT  connectionContext,
								IN ULONG  receiveFlags,
								IN ULONG  bytesIndicated,
								IN ULONG  bytesAvailable,
								OUT ULONG  *bytesTaken,
								IN PVOID  tsdu,
								OUT PIRP  *ioRequestPacket
								)
{
	NTSTATUS status;

	EventHandler* event_handler = (EventHandler*)tdiEventContext;

	DbgPrint("TdiReceiveEventHandler\t\t\tNode = %08x \n", event_handler->node);

	status = ((PTDI_IND_RECEIVE)event_handler->original_handler.EventHandler)(
		event_handler->original_handler.EventContext,
		connectionContext,
		receiveFlags,
		bytesIndicated,
		bytesAvailable,
		bytesTaken,
		tsdu,
		ioRequestPacket);


	return status;
}

NTSTATUS TdiReceiveDatagramEventHandler(
										IN PVOID TdiEventContext,
										IN LONG SourceAddressLength,
										IN PVOID SourceAddress,
										IN LONG OptionsLength,
										IN PVOID Options,
										IN ULONG ReceiveDatagramFlags,
										IN ULONG BytesIndicated,
										IN ULONG BytesAvailable,
										OUT ULONG *BytesTaken,
										IN PVOID Tsdu,
										OUT PIRP *IoRequestPacket
										)
{
	NTSTATUS status;

	EventHandler* event_handler = (EventHandler*)TdiEventContext;

	DbgPrint("TdiReceiveDatagramEventHandler\t\t\tNode = %08x \n", event_handler->node);
	status = ((PTDI_IND_RECEIVE_DATAGRAM)event_handler->original_handler.EventHandler)(
		event_handler->original_handler.EventContext,
		SourceAddressLength,
		SourceAddress,
		OptionsLength,
		Options,
		ReceiveDatagramFlags,
		BytesIndicated,
		BytesAvailable,
		BytesTaken,
		Tsdu,
		IoRequestPacket);
	return status;
}

NTSTATUS TdiReceiveExpeditedEventHandler(
	IN PVOID  TdiEventContext,
	IN CONNECTION_CONTEXT  ConnectionContext,
	IN ULONG  ReceiveFlags,
	IN ULONG  BytesIndicated,
	IN ULONG  BytesAvailable,
	OUT ULONG  *BytesTaken,
	IN PVOID  Tsdu,
	OUT PIRP  *IoRequestPacket)
{
	NTSTATUS status;

	EventHandler* event_handler = (EventHandler*)TdiEventContext;

	DbgPrint("TdiReceiveExpeditedEventHandler\t\t\tNode = %08x \n", event_handler->node);

	status = ((PTDI_IND_RECEIVE_EXPEDITED)event_handler->original_handler.EventHandler)(
		event_handler->original_handler.EventContext,
		ConnectionContext,
		ReceiveFlags,
		BytesIndicated,
		BytesAvailable,
		BytesTaken,
		Tsdu,
		IoRequestPacket);

	return status;
}

NTSTATUS TdiSendPossibleEventHandler(
									 IN PVOID  TdiEventContext,
									 IN PVOID  ConnectionContext,
									 IN ULONG  BytesAvailable)
{
	NTSTATUS status;

	EventHandler* event_handler = (EventHandler*)TdiEventContext;

	DbgPrint("TdiSendPossibleEventHandler\t\t\tNode = %08x \n", event_handler->node);

	status = ((PTDI_IND_SEND_POSSIBLE)event_handler->original_handler.EventHandler)(
		event_handler->original_handler.EventContext,
		ConnectionContext,
		BytesAvailable);

	return status;
}

NTSTATUS TdiChainedReceiveEventHandler(
									   IN PVOID  TdiEventContext,
									   IN CONNECTION_CONTEXT  ConnectionContext,
									   IN ULONG  ReceiveFlags,
									   IN ULONG  ReceiveLength,
									   IN ULONG  StartingOffset,
									   IN PMDL  Tsdu,
									   IN PVOID  TsduDescriptor)
{
	NTSTATUS status;

	EventHandler* event_handler = (EventHandler*)TdiEventContext;

	DbgPrint("TdiChainedReceiveEventHandler\t\t\tNode = %08x \n", event_handler->node);

	status = ((PTDI_IND_CHAINED_RECEIVE)event_handler->original_handler.EventHandler)(
		event_handler->original_handler.EventContext,
		ConnectionContext,
		ReceiveFlags,
		ReceiveLength,
		StartingOffset,
		Tsdu,
		TsduDescriptor);

	return status;
}

NTSTATUS TdiChainedReceiveDatagramEventHandler(
	IN PVOID  TdiEventContext,
	IN LONG  SourceAddressLength,
	IN PVOID  SourceAddress,
	IN LONG  OptionsLength,
	IN PVOID  Options,
	IN ULONG  ReceiveDatagramFlags,
	IN ULONG  ReceiveDatagramLength,
	IN ULONG  StartingOffset,
	IN PMDL  Tsdu,
	IN PVOID  TsduDescriptor)
{
	NTSTATUS status;

	EventHandler* event_handler = (EventHandler*)TdiEventContext;

	DbgPrint("TdiChainedReceiveDatagramEventHandler\t\t\tNode = %08x \n", event_handler->node);

	status = ((PTDI_IND_CHAINED_RECEIVE_DATAGRAM)event_handler->original_handler.EventHandler)(
		event_handler->original_handler.EventContext,
		SourceAddressLength,
		SourceAddress,
		OptionsLength,
		Options,
		ReceiveDatagramFlags,
		ReceiveDatagramLength,
		StartingOffset,
		Tsdu,
		TsduDescriptor);

	return status;
}

NTSTATUS TdiChainedReceiveExpeditedEventHandler(
	IN PVOID  TdiEventContext,
	IN CONNECTION_CONTEXT  ConnectionContext,
	IN ULONG  ReceiveFlags,
	IN ULONG  ReceiveLength,
	IN ULONG  StartingOffset,
	IN PMDL  Tsdu,
	IN PVOID  TsduDescriptor)
{
	NTSTATUS status;

	EventHandler* event_handler = (EventHandler*)TdiEventContext;

	DbgPrint("TdiChainedReceiveExpeditedEventHandler\t\t\tNode = %08x \n", event_handler->node);

	status = ((PTDI_IND_CHAINED_RECEIVE_EXPEDITED)event_handler->original_handler.EventHandler)(
		event_handler->original_handler.EventContext,
		ConnectionContext,
		ReceiveFlags,
		ReceiveLength,
		StartingOffset,
		Tsdu,
		TsduDescriptor);

	return status;
}

NTSTATUS TdiErrorExEventHandler(
								IN PVOID  TdiEventContext,
								IN NTSTATUS  Status,
								IN PVOID  Buffer)
{
	NTSTATUS status;

	EventHandler* event_handler = (EventHandler*)TdiEventContext;

	DbgPrint("TdiErrorExEventHandler\t\t\tNode = %08x \n", event_handler->node);

	status = ((PTDI_IND_ERROR_EX)event_handler->original_handler.EventHandler)(
		event_handler->original_handler.EventContext,
		Status,
		Buffer);

	return status;
}