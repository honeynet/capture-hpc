/*
*	PROJECT: Capture
*	FILE: CaptureTdiIrpHandlers.c
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
*	(at your option) any later version.
*
*	Capture is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with Capture; if not, write to the Free Software
*	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "CaptureTdiIrpHandlers.h"

#include "CaptureConnectionMonitor.h"
#include "Connection.h"
#include "ConnectionManager.h"

const CHAR* TdiEventHandlerNames[] =
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

const CHAR* TdiDisconnectFlagNames[] =
{
	"DEFAULT",
	"TDI_DISCONNECT_WAIT",
	"TDI_DISCONNECT_ABORT",
	"",
	"TDI_DISCONNECT_RELEASE"
};

// TDI IRP handlers
NTSTATUS TdiDefaultHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	return STATUS_SUCCESS;
}

NTSTATUS TdiAssociateAddressHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	NTSTATUS status;
	HANDLE local_address_handle = ((TDI_REQUEST_KERNEL_ASSOCIATE *)(&irp_stack->Parameters))->AddressHandle;

	if( local_address_handle != NULL )
	{
		PFILE_OBJECT local_node = NULL;

		status = ObReferenceObjectByHandle(local_address_handle, GENERIC_READ, NULL, KernelMode, &local_node, NULL);

		if( NT_SUCCESS(status) )
		{
			Connection* connection = NULL;
			ConnectionNode* local_connection_node = NULL;
			ConnectionNode* remote_connection_node = NULL;
			PFILE_OBJECT remote_node = irp_stack->FileObject;

			DbgPrint("TdiAssociateAddressHandler\t\t\tLocal node = %08x Remote node = %08x\n", local_node, remote_node);

			remote_connection_node = (ConnectionNode*)FindHashMapValue(&connection_manager->connections, (UINT)remote_node);
			local_connection_node = (ConnectionNode*)FindHashMapValue(&connection_manager->connections, (UINT)local_node);

			connection = CreateConnection(connection_manager);

			if(connection && remote_connection_node && local_connection_node)
			{
				SetLocalNode(connection, local_node);
				SetRemoteNode(connection, remote_node);
				//One local can be used in many connection
				//local_connection_node->connection = connection;
				remote_connection_node->connection = connection;

				// Get the process id that initiated the irp
				// Should be the userland process, but could be the system for kernel drivers (netbt, afd etc)
				connection->process_id = IoGetRequestorProcessId(irp);
			}
			else
			{
				DbgPrint("TdiAssociateAddressHandler\t\t\tConnection node not found\n");
			}

			ObDereferenceObject(local_node);
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS TdiDisassociateAddressHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	Connection* connection = NULL;

	connection = GetConnection(connection_manager, irp_stack->FileObject);

	if(connection == NULL)
		return STATUS_SUCCESS;

	ReleaseConnectionNode(connection_manager, connection, irp_stack->FileObject);
	RemoveConnection(connection_manager, connection);
	return STATUS_SUCCESS;
}

NTSTATUS TdiConnectHandlerComplete( PDEVICE_OBJECT device_object, PIRP irp, PVOID context )
{
	ConnectionManager* connection_manager = NULL;
	Connection* connection = (Connection*)context;

	connection_manager = (ConnectionManager*)device_object->DeviceExtension;

	if( irp->IoStatus.Status == STATUS_SUCCESS )
	{
		if (connection_manager->device_type==TCP_DEVICE)
			SetConnectionState( connection_manager, connection, TCP_STATE_ESTABLISHED);
		else if (connection_manager->device_type==UDP_DEVICE)
			SetConnectionState( connection_manager, connection, UDP_STATE_ESTABLISHED);
	}

	return STATUS_SUCCESS;
}

NTSTATUS TdiConnectHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	ConnectionNode* connection_node = NULL;
	PTA_ADDRESS remote_address = NULL;
	PFILE_OBJECT remote_node = irp_stack->FileObject;
	PTDI_REQUEST_KERNEL_CONNECT connect_info = NULL;

	connection_node = (ConnectionNode*)FindHashMapValue(&connection_manager->connections, (UINT)remote_node);
	connect_info = (PTDI_REQUEST_KERNEL_CONNECT)&irp_stack->Parameters;
	if(connection_node == NULL)
	{
		DbgPrint("TdiConnectHandler\t\t\tNode not found: %08x\n", remote_node);
		return STATUS_SUCCESS;
	}
	//UDP connection: associated connection has not created yet
	if (connection_manager->device_type==UDP_DEVICE)
	{
		Connection * connection=CreateConnection(connection_manager);
		SetLocalNode(connection,connection_node->node);
		SetRemoteNode(connection,connection_node->node);
		connection->process_id=IoGetRequestorProcessId(irp);
		connection_node->connection=connection;
	}
	if(connection_node->connection == NULL)
	{
		DbgPrint("TdiConnectHandler\t\t\tConnection does not exist for node: %08x\n", remote_node);
		return STATUS_SUCCESS;
	}
	
	DbgPrint("TdiConnectHandler\t\t\tGot connection for node: %08x\n", remote_node);
	
	if(connect_info)
	{
		remote_address = ((TRANSPORT_ADDRESS*)(connect_info->RequestConnectionInformation->RemoteAddress))->Address;

		if(remote_address && remote_address->AddressType == TDI_ADDRESS_TYPE_IP)
		{
			SetRemoteNodeAddress(connection_node->connection, remote_address);
		}
	}

	GetLocalAddress(connection_node->connection, connection_manager->next_device);

	if (connection_manager->device_type==UDP_DEVICE)
		SetConnectionState(connection_manager, connection_node->connection, UDP_STATE_CONNECT);
	else SetConnectionState(connection_manager, connection_node->connection, TCP_STATE_SYN_SENT);

	completion_routine->callback = TdiConnectHandlerComplete;
	completion_routine->context = connection_node->connection;
	completion_routine->control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;

	return STATUS_SUCCESS;
}

NTSTATUS TdiCreateHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	PFILE_FULL_EA_INFORMATION ea_information;
	PTRANSPORT_ADDRESS ptaddress=NULL;
	ea_information = irp->AssociatedIrp.SystemBuffer;

	if(ea_information == NULL)
	{
		DbgPrint("TdiCreateHandler\t\t\tGot: Control Channel Object\n");
	}
	else if(MmIsAddressValid(ea_information) && 
		MmIsAddressValid(ea_information->EaName))
	{
		ULONG name_length = ea_information->EaNameLength;

		if(name_length == TDI_TRANSPORT_ADDRESS_LENGTH && 
			memcmp(ea_information->EaName, TdiTransportAddress, TDI_TRANSPORT_ADDRESS_LENGTH) == 0)
		{
			DbgPrint("TdiCreateHandler\t\t\tGot: Transport Address Object\n");
			ptaddress= (PTRANSPORT_ADDRESS)(&ea_information->EaName+TDI_TRANSPORT_ADDRESS_LENGTH+1);
			PrintAddress(ptaddress->Address);
			AddConnectionNode(connection_manager, NULL, irp_stack->FileObject);
		}
		else if(name_length == TDI_CONNECTION_CONTEXT_LENGTH &&
			memcmp(ea_information->EaName, TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH) == 0)
		{
			DbgPrint("TdiCreateHandler\t\t\tGot: Connection Context Object\n");

			AddConnectionNode(connection_manager, NULL, irp_stack->FileObject);
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS TdiCloseHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	RemoveConnectionNode(connection_manager,irp_stack->FileObject);
	return STATUS_SUCCESS;
}

NTSTATUS TdiCleanupHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	//RemoveConnection(connection_)
	return STATUS_SUCCESS;
}


NTSTATUS TdiListenHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	return STATUS_SUCCESS;
}

NTSTATUS TdiAcceptHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	return STATUS_SUCCESS;
}

NTSTATUS TdiDisconnectHandlerComplete( PDEVICE_OBJECT device_object, PIRP irp, PVOID context )
{
	ConnectionManager* connection_manager = NULL;
	Connection* connection = (Connection*)context;

	connection_manager = (ConnectionManager*)device_object->DeviceExtension;

	if( irp->IoStatus.Status == STATUS_SUCCESS )
	{
		// Not sure if this is the correct place to set this state
		// This is assuming the TDI_DISCONNECT irp receives an ACK to set this state
		//SetConnectionState( connection_manager, connection, TCP_STATE_FIN_WAIT_2);
	}

	return STATUS_SUCCESS;
}


NTSTATUS TdiDisconnectHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	PTDI_REQUEST_KERNEL_DISCONNECT disconnect_information = NULL;
	Connection* connection = GetConnection(connection_manager, irp_stack->FileObject);

	if(connection == NULL)
		return STATUS_SUCCESS;

	disconnect_information = (PTDI_REQUEST_KERNEL_DISCONNECT)&irp_stack->Parameters;

	DbgPrint("TdiDisconnectHandler\t\t\tNode = %08x %s connection=%08x\n", irp_stack->FileObject, TdiDisconnectFlagNames[disconnect_information->RequestFlags], connection);

	if(disconnect_information->RequestFlags == TDI_DISCONNECT_ABORT)
	{
		// Sending RST
		SetConnectionState(connection_manager, connection, TCP_STATE_CLOSED);
	}
	else if (disconnect_information->RequestFlags == TDI_DISCONNECT_RELEASE)
	{
		// Sending FIN
		if(connection->state == TCP_STATE_CLOSE_WAIT)
		{
			SetConnectionState( connection_manager, connection, TCP_STATE_LAST_ACK);
		}
	}


	//if(connection->state == TCP_STATE_ESTABLISHED)
	//{
	//	SetConnectionState( connection_manager, connection, TCP_STATE_FIN_WAIT_1);

	//	completion_routine->callback = TdiDisconnectHandlerComplete;
	//	completion_routine->context = connection;
	//	completion_routine->control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;
	//}
	//else if(connection->state == TCP_STATE_CLOSE_WAIT)
	//{
	//	SetConnectionState( connection_manager, connection, TCP_STATE_LAST_ACK);
	//}
	//else if(connection->state == TCP_STATE_SYN_RCVD)
	//{
	//	SetConnectionState( connection_manager, connection, TCP_STATE_FIN_WAIT_1);
	//}
	
	return STATUS_SUCCESS;
}

NTSTATUS TdiSendHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	ConnectionNode* connection_node = NULL;
	PFILE_OBJECT node = irp_stack->FileObject;
	PTDI_REQUEST_KERNEL_CONNECT connect_info = NULL;

	connection_node = (ConnectionNode*)FindHashMapValue(&connection_manager->connections, (UINT)node);

	if(connection_node == NULL)
	{
		DbgPrint("TdiSendHandler\t\t\tNode not found: %08x\n", node);
		return STATUS_SUCCESS;
	}

	if(connection_node->connection == NULL)
	{
		DbgPrint("TdiSendHandler\t\t\tConnection does not exist for node: %08x\n", node);
		return STATUS_SUCCESS;
	}

	GetLocalAddress(connection_node->connection, connection_manager->next_device);

	return STATUS_SUCCESS;
}

NTSTATUS TdiReceiveHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	return STATUS_SUCCESS;
}

NTSTATUS TdiSendDatagramHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	ConnectionNode* connection_node=NULL;
	Connection* connection=NULL;
	PTA_ADDRESS remote_address=NULL;
	PFILE_OBJECT node = irp_stack->FileObject;
	TDI_REQUEST_KERNEL_SENDDG *connect_info = (TDI_REQUEST_KERNEL_SENDDG *) (&irp_stack->Parameters);
	//search for connection node
	connection_node = (ConnectionNode*)FindHashMapValue(&connection_manager->connections, (UINT)node);	
	if (connection_node==NULL)
	{
		DbgPrint("SEND DATAGRAM HANDLER: Can't find node %08x\n", node);
		return STATUS_SUCCESS;
	}
	else 
	{
		if (connection_node->connection==NULL)
		{
			connection_node->connection=CreateConnection(connection_manager);
			connection_node->connection->local_node=connection_node->node;
			connection_node->connection->remote_node=NULL;
			connection_node->connection->process_id = IoGetRequestorProcessId(irp);
		}
		GetLocalAddress(connection_node->connection, connection_manager->next_device);
		remote_address=((TRANSPORT_ADDRESS*)(connect_info->SendDatagramInformation->RemoteAddress))->Address;
		SetRemoteNodeAddress(connection_node->connection, remote_address);
		SetConnectionState(connection_manager, connection_node->connection, UDP_STATE_SEND_DATAGRAM);
	}
	return STATUS_SUCCESS;
}

NTSTATUS TdiReceiveDatagramHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	return STATUS_SUCCESS;
}

NTSTATUS TdiSetEventHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{	
	ConnectionNode* connection_node=NULL;
	Connection* connection=NULL;
	PFILE_OBJECT node = irp_stack->FileObject;
	PTDI_REQUEST_KERNEL_SET_EVENT event_handler = (PTDI_REQUEST_KERNEL_SET_EVENT)&irp_stack->Parameters;

	if( event_handler->EventHandler == NULL &&
		event_handler->EventContext == NULL )
	{
		DbgPrint("TdiSetEventHandler\t\t\t%08x - Remove event handler = %s\n", irp_stack->FileObject, TdiEventHandlerNames[event_handler->EventType]);

		RemoveEventHandler(&connection_manager->event_handler_manager, event_handler, node);
	}
	else
	{
		DbgPrint("TdiSetEventHandler\t\t\t%08x - Set event handler = %s\n", irp_stack->FileObject, TdiEventHandlerNames[event_handler->EventType]);

		SetEventHandler(&connection_manager->event_handler_manager, event_handler, node);

		// If we are monitoring connect events then the connection is listening for connection
		if(event_handler->EventType == TDI_EVENT_CONNECT)
		{
			//Connection* connection = GetConnection(connection_manager, node);
			//Create a special connection for listen state
			connection_node=(ConnectionNode*)FindHashMapValue(&connection_manager->connections, (UINT)node);
			connection = CreateConnection(connection_manager);
			SetLocalConnectionNode(connection_manager, connection, node);
			SetRemoteConnectionNode(connection_manager, connection, node);
			connection_node->connection=connection;
			
			GetLocalAddress(connection, connection_manager->next_device);
			SetRemoteNodeAddress(connection, (PTA_ADDRESS)connection->local_node_address);
			SetConnectionState( connection_manager, connection, TCP_STATE_LISTEN );
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS TdiQueryInformationHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	return STATUS_SUCCESS;
}

NTSTATUS TdiSetInformationHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	return STATUS_SUCCESS;
}

NTSTATUS TdiActionHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	return STATUS_SUCCESS;
}