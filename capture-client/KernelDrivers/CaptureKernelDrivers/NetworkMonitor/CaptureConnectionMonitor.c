/*
 *	PROJECT: Capture
 *	FILE: CaptureConnectionMonitor.c
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
#include "CaptureConnectionMonitor.h"

#include <strsafe.h>

KIRQL g_irq_level;

//NTSTATUS TdiCall(IN PIRP pIrp,
//                 IN PDEVICE_OBJECT pDeviceObject,
//                 IN OUT PIO_STATUS_BLOCK pIoStatusBlock)
//{
//    KEVENT kEvent;                                                  
//    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;              
//
//    KeInitializeEvent ( &kEvent, NotificationEvent, FALSE ); 
//
//    pIrp->UserEvent = &kEvent;                                   
//    pIrp->UserIosb = pIoStatusBlock;                           
//    ntStatus = IoCallDriver ( pDeviceObject, pIrp ); 
//
//    if(ntStatus == STATUS_PENDING)                                
//    {
//        KeWaitForSingleObject ( 
//            (PVOID)&kEvent,                             
//            Executive,                                   
//            KernelMode,                                 
//            TRUE,                                       
//            NULL ); 
//
//        ntStatus = pIoStatusBlock->Status; 
//    }    
//
//    return ( ntStatus );                                             
//}
//
void CopyAddress( PTA_ADDRESS* destination, PTA_ADDRESS source )
{
	//int size = sizeof(TDI_ADDRESS_INFO) + sizeof(TDI_ADDRESS_IP);
	
	int size = sizeof(TA_ADDRESS) + source->AddressLength;	

	if( *destination == NULL )
	{
		*destination = (PTA_ADDRESS)ExAllocatePoolWithTag(NonPagedPool, size, CONNECTION_POOL_TAG);
	}

	if( *destination == NULL )
	{
		return;
	}

	RtlCopyMemory( *destination, source, size );
}

NTSTATUS GetLocalAddressComplete( PDEVICE_OBJECT device_object, PIRP irp, PVOID context )
{
	Connection* connection = (Connection*)context;

	if( irp->IoStatus.Status != STATUS_SUCCESS )
	{
		ExFreePoolWithTag( connection->local_address, CONNECTION_POOL_TAG );
		connection->local_address = NULL;
	}

	if( irp->MdlAddress )
	{
		IoFreeMdl( irp->MdlAddress );
	}

	return STATUS_SUCCESS;
}

void GetLocalAddressWorkItem( PVOID user_data )
{
	AddressWorkItem* work_item = (AddressWorkItem*)user_data;

	GetLocalAddress( work_item->device_object, work_item->connection );

	ExFreePoolWithTag( work_item, CONNECTION_POOL_TAG );
}

void GetLocalAddress( PDEVICE_OBJECT device_object, Connection* connection )
{
	UINT size = 0;
	PMDL mdl = NULL;
	NTSTATUS status;
	KEVENT kernel_event;
	PIRP query_irp = NULL;
	IO_STATUS_BLOCK io_status_block;
	PTDI_ADDRESS_INFO local_address = NULL;
	PFILE_OBJECT connection_object = connection->local_node;

    if(KeGetCurrentIrql() != PASSIVE_LEVEL)
    {
		AddressWorkItem* address_work = (AddressWorkItem*)ExAllocatePoolWithTag( NonPagedPool, sizeof(AddressWorkItem), CONNECTION_POOL_TAG );

		if( address_work )
		{
			RtlZeroMemory( address_work, sizeof(AddressWorkItem) );

			address_work->device_object = device_object;
			address_work->connection = connection;

			// Initialise our delayed work item that will be executed at the PASSIVE_LEVEL
			// Note, we should probably use IoAllocateWorkItem because the device may disappear before
			// the item is executed?
			ExInitializeWorkItem( &address_work->work_item, GetLocalAddressWorkItem, address_work );

			ExQueueWorkItem( &address_work->work_item, DelayedWorkQueue );	
		}

		return;
    }

	//RtlZeroMemory(

	size = sizeof(TDI_ADDRESS_INFO) + sizeof(TDI_ADDRESS_IP);

	local_address = (PTDI_ADDRESS_INFO)ExAllocatePoolWithTag( PagedPool, size, CONNECTION_POOL_TAG );

	if( local_address == NULL )
		goto error;

	query_irp = TdiBuildInternalDeviceControlIrp( TDI_QUERY_INFORMATION, device_object, connection_object, NULL, NULL );

	if( query_irp == NULL )
		goto error;

	KeInitializeEvent ( &kernel_event, NotificationEvent, FALSE );
	query_irp->UserEvent = &kernel_event;
	query_irp->UserIosb = &io_status_block;

	mdl = IoAllocateMdl( local_address, size, FALSE, FALSE, NULL );

	if( mdl == NULL )
		goto error;

	__try
	{
		MmProbeAndLockPages( mdl, KernelMode, IoModifyAccess );   
	} 
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		IoFreeMdl( mdl );
		mdl = NULL;
		goto error;
	}

	TdiBuildQueryInformation( query_irp, device_object, connection_object, NULL, NULL, TDI_QUERY_ADDRESS_INFO, mdl);   
	  
	status = IoCallDriver( device_object, query_irp ); 

	if(status == STATUS_PENDING)                                
    {
        KeWaitForSingleObject( (PVOID)&kernel_event, Executive, KernelMode, TRUE, NULL ); 
        status = io_status_block.Status; 
    }

	if( status == STATUS_SUCCESS )
	{
		CopyAddress( &connection->local_address, local_address->Address.Address );
	} 
	else
	{
		connection->local_address = NULL;
	}

	ExFreePoolWithTag( local_address, CONNECTION_POOL_TAG );

	return;

error:

	if( mdl )						IoFreeMdl( mdl );
	if( query_irp )					IoCompleteRequest( query_irp, IO_NO_INCREMENT );
	if( local_address )	ExFreePoolWithTag( local_address, CONNECTION_POOL_TAG );

	return;
}

NTSTATUS ClientEventConnectComplete(IN PDEVICE_OBJECT device_object, IN PIRP irp, IN PVOID context)
{
	NTSTATUS status = STATUS_SUCCESS;
	ClientEventConnectContext* connection_context = (ClientEventConnectContext*)context;

	if( irp->IoStatus.Status == STATUS_SUCCESS )
	{
		connection_context->connection->state = TCP_STATE_ESTABLISHED;
	}

	status = connection_context->previous_callback.callback( device_object, irp, connection_context->previous_callback.context );

	return status;
}

NTSTATUS ClientEventConnect(
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
	ULONG process_id;
	
	ConnectionContext* connection_context = (ConnectionContext*)tdiEventContext;

	TA_ADDRESS *remote_address = ((TRANSPORT_ADDRESS *)remoteAddress)->Address;

	TDI_ADDRESS_IP* ip_address = (TDI_ADDRESS_IP*)remote_address->Address;
	char *p;
	p = (char *)&ip_address->in_addr;
	DbgPrint("ClientEventConnect: %08x \n", connection_context->file_object);
	DbgPrint("\tSource: %d.%d.%d.%d:%i\n", (UCHAR)p[0], (UCHAR)p[1], (UCHAR)p[2], (UCHAR)p[3], ip_address->sin_port);  

	process_id = 0;
	
	status = ((PTDI_IND_CONNECT)connection_context->original_callback)(
		connection_context->original_context,
		remoteAddressLength,
		remoteAddress,
		userDataLength,
		userData,
		optionsLength,
		options,
		connectionContext,
		acceptIrp);

	 if( status == STATUS_MORE_PROCESSING_REQUIRED )
	 {
		 // We don't bother with this as we aren't monitoring the actual state of
		 // connections. If the connect fails we will remove the connection when
		 // a TDI_DISASSOCIATE irp comes
	 }

	if( status == STATUS_SUCCESS && *acceptIrp != NULL)
	{
		Pair* pair = NULL;
		Connection* connection = NULL;
		PIO_STACK_LOCATION irp_stack = NULL;

		irp_stack = IoGetCurrentIrpStackLocation( *acceptIrp );
		
		pair = FindHashMap( &connection_context->connection_manager->connection_hash_map, (UINT)irp_stack->FileObject);

		DbgPrint("\t\t\t\tFind: %08x\n", irp_stack->FileObject);

		if( pair )
		{
			connection = (Connection*)pair->value;

			connection->state = TCP_STATE_SYN_RCVD;

			DbgPrint("\t\t\t\tFound: %08x - l=%08x c=%08x p=%i\n", connection, connection->local_node, connection->connection, connection->process_id);

			if(remote_address && remote_address->AddressType == TDI_ADDRESS_TYPE_IP)
			{
				CopyAddress( &connection->remote_address, remote_address  );
			}
		}
		else
		{
			DbgPrint("\t\t\t\tCould not Find: %08x\n", irp_stack->FileObject);
		}
	}
	
	return status;
}

NTSTATUS ClientEventDisconnect(
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
	ULONG process_id;
	
	ConnectionContext* connection_context = (ConnectionContext*)tdiEventContext;
	//DbgBreakPoint();
	//process_id = IoGetRequestorProcessId(irp);
	//DbgPrint("ClientEventDisconnect: P=%i \n", process_id);
	DbgPrint("ClientEventDisconnect: %08x \n", connection_context->file_object);

	status = ((PTDI_IND_DISCONNECT)connection_context->original_callback)(
		connection_context->original_context,
		connectionContext,
		disconnectDataLength,
		disconnectData,
		disconnectInformationLength,
		disconnectInformation,
		disconnectFlags);

	return status;
}

NTSTATUS ClientEventReceive(
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
	ULONG process_id;
	
	ConnectionContext* connection_context = (ConnectionContext*)tdiEventContext;

	DbgPrint("ClientEventReceive: %08x \n", connection_context->file_object);

	status = ((PTDI_IND_RECEIVE)connection_context->original_callback)(
		connection_context->original_context,
		connectionContext,
		receiveFlags,
		bytesIndicated,
		bytesAvailable,
		bytesTaken,
		tsdu,
		ioRequestPacket);

	
	return status;
}

Connection* CreateConnection( ConnectionManager* connection_manager, int process_id, PFILE_OBJECT local, PFILE_OBJECT remote )
{
	Connection* connection = (Connection*)ExAllocatePoolWithTag( NonPagedPool, sizeof(Connection), CONNECTION_POOL_TAG );

	if( connection == NULL )
	{
		return NULL;
	}

	RtlZeroMemory( connection, sizeof(Connection) );

	// Start Lock
	KeAcquireSpinLock( &connection_manager->lock, &g_irq_level );

	connection->prev = connection_manager->connection_tail;

	if( connection_manager->connection_head == NULL )
	{
		connection_manager->connection_head = connection;
	}

	if( connection_manager->connection_tail != NULL )
	{
		connection_manager->connection_tail->next = connection;
	}

	connection_manager->connection_tail = connection;

	InsertHashMap( &connection_manager->connection_hash_map, (UINT)local, connection );

	KeReleaseSpinLock( &connection_manager->lock, g_irq_level );
	// End Lock

	connection->local_node = local;
	connection->connection = remote;
	connection->process_id = process_id;

	return connection;
}

void FreeConnection( ConnectionManager* connection_manager, Connection* connection )
{
	if( connection == NULL )
	{
		return;
	}

	if( connection->next != NULL )
	{
		connection->next->prev = connection->prev;
	}

	if( connection->prev != NULL )
	{
		connection->prev->next = connection->next;
	}

	if( connection_manager->connection_head == connection )
	{
		connection_manager->connection_head = connection->next;
	}

	if( connection_manager->connection_tail == connection )
	{
		connection_manager->connection_tail = connection->prev;
	}

	if( connection->remote_address != NULL )
	{
		ExFreePoolWithTag( connection->remote_address, CONNECTION_POOL_TAG );
	}

	if( connection->local_address != NULL )
	{
		ExFreePoolWithTag( connection->local_address, CONNECTION_POOL_TAG );
	}

	ExFreePoolWithTag( connection, CONNECTION_POOL_TAG );
}



void VPrintf( char* buffer, int buffer_size, const char* format, ... )
{
	va_list arglist;
	va_start(arglist, format);

	StringCchVPrintfA( buffer, buffer_size, "%d.%d.%d.%d:%i", arglist);

	va_end(arglist);
}

char* no_address = "0.0.0.0:-1";

void AddressToString( PTA_ADDRESS address, char* buffer, int buffer_size )
{			
	//PTA_ADDRESS address = NULL;

	if( address == NULL )
	{
		//RtlCopyMemory( buffer, no_address, strlen(no_address) +);
		StringCchCopyA( buffer, buffer_size, no_address );
		return;
	}

	//address = address_info->Address.Address;

	if(address->AddressType == TDI_ADDRESS_TYPE_IP)
	{
		TDI_ADDRESS_IP* ip_address = (TDI_ADDRESS_IP*)address->Address;
		char *p;
		p = (char *)&ip_address->in_addr;

		VPrintf( buffer, buffer_size, "%d.%d.%d.%d:%i", (UCHAR)p[0], (UCHAR)p[1], (UCHAR)p[2], (UCHAR)p[3], ip_address->sin_port);
	}
}

void DbgPrintConnection( Connection* connection )
{
	char local_address[24];
	char remote_address[24];

	if( connection == NULL )
	{
		return;
	}

	AddressToString( connection->local_address, local_address, 24 );
	AddressToString( connection->remote_address, remote_address, 24 );

	DbgPrint("\t\t\t\tConnection: %08x - l=%08x c=%08x p=%i local=%s remote=%s\n", connection, connection->local_node, connection->connection, connection->process_id, local_address, remote_address);
}

NTSTATUS ConnectCompletion( IN PDEVICE_OBJECT  device_object,
					   IN PIRP  irp,
					   IN PVOID  context )
{
	PIO_STACK_LOCATION pirp_stack; 

	pirp_stack = IoGetCurrentIrpStackLocation(irp);
	//GetLocalAddress(device_object, pirp_stack->FileObject);

	return STATUS_SUCCESS;
}

void RemoveConnection( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack )
{
	Pair* pair = NULL;
	Connection* connection = NULL;
	
	pair = FindHashMap( &connection_manager->connection_hash_map, (UINT)irp_stack->FileObject);
	DbgPrint("\t\t\t\tFind: %08x\n", irp_stack->FileObject);
	if( pair )
	{
		connection = (Connection*)pair->value;
		DbgPrint("\t\t\t\tFound: %08x - l=%08x c=%08x p=%i\n", connection, connection->local_node, connection->connection, connection->process_id);
	}
	else
	{
		DbgPrint("\t\t\t\tCould not Find: %08x\n", irp_stack->FileObject);
	}

	if( connection )
	{
		// Start Lock
		KeAcquireSpinLock( &connection_manager->lock, &g_irq_level );

		if( RemoveHashMap( &connection_manager->connection_hash_map, (UINT)irp_stack->FileObject ) )
		{
			DbgPrint("\t\t\t\tConnection Removed: %08x\n", irp_stack->FileObject);
		}
		else
		{
			DbgPrint("\t\t\t\tCould not Find - Remove: %08x\n", irp_stack->FileObject);
		}

		FreeConnection( connection_manager, connection );

		KeReleaseSpinLock( &connection_manager->lock, g_irq_level );
		// End Lock
	}
}

NTSTATUS HandleIrp( IN PDEVICE_OBJECT device_object, 
				    IN PIRP irp )
{ 
	NTSTATUS status;
	ULONG process_id;
	PIO_STACK_LOCATION pirp_stack; 
	CompletionRoutine completion_routine;
	ConnectionManager* connection_manager = NULL;

	connection_manager = (ConnectionManager*)device_object->DeviceExtension;
	
	pirp_stack = IoGetCurrentIrpStackLocation(irp);

	completion_routine.callback = NULL;
	completion_routine.context = NULL;

	if(pirp_stack->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL)
	{
		int tdi_irp = pirp_stack->MinorFunction;

		DbgPrint("TDI IRP = %s\t\t%08x\n", TdiIrpNames[pirp_stack->MinorFunction], pirp_stack->FileObject);	

		if( tdi_irp < num_tdi_irp_handlers )
		{
			((TdiIrpHandler)(tdi_irp_handlers[tdi_irp]))( connection_manager, irp, pirp_stack, &completion_routine );
		}
	}
	else if( pirp_stack->MajorFunction == IRP_MJ_CLEANUP )
	{
		
	}

	if( completion_routine.callback )
	{
		CompletionRoutineContext* context = (CompletionRoutineContext*)ExAllocatePoolWithTag( NonPagedPool, sizeof(CompletionRoutineContext), CONNECTION_POOL_TAG );
		
		if( context != NULL )
		{
			RtlZeroMemory( context, sizeof(CompletionRoutineContext) );

			context->completion_routine.callback = completion_routine.callback;
			context->completion_routine.context = completion_routine.context;
			context->completion_routine.control = completion_routine.control;

			// Not enough stack locations exists to put our completion routine in
			// We create a new irp with enough locations and send that down to the
			// next lowest driver. In the completion handler we call the callback,
			// free the irp we created and complete the original irp which calls
			// its completion routine.
			// We do this because drivers like netbt use legacy code that don't
			// have any free stack locations
			if( irp->CurrentLocation <= 1 )
			{
				PIRP new_irp = NULL;
				PIO_STACK_LOCATION new_sirp = NULL;	

				new_irp = IoAllocateIrp(connection_manager->next_device->StackSize, FALSE);

				if( new_irp == NULL )
				{
					// Pass the irp as normal - our completion routine will not be called
					IoSkipCurrentIrpStackLocation( irp );
					return IoCallDriver(connection_manager->next_device, irp);
				}

				// Copy the original irp state to the newly created irp
				context->original_irp = irp;

				new_irp->MdlAddress = irp->MdlAddress;
				new_irp->Flags = irp->Flags;
				new_irp->AssociatedIrp = irp->AssociatedIrp;
				new_irp->RequestorMode = irp->RequestorMode;
				new_irp->UserIosb = irp->UserIosb;
				new_irp->UserEvent = irp->UserEvent;
				new_irp->Overlay = irp->Overlay;
				new_irp->UserBuffer = irp->UserBuffer;
				new_irp->Tail.Overlay.AuxiliaryBuffer = irp->Tail.Overlay.AuxiliaryBuffer;
				new_irp->Tail.Overlay.OriginalFileObject = irp->Tail.Overlay.OriginalFileObject;
				new_irp->Tail.Overlay.Thread = irp->Tail.Overlay.Thread;

				// Give our new irp the same stack location as the original
				new_sirp = IoGetNextIrpStackLocation(new_irp);
				*new_sirp = *pirp_stack;

				IoSetCompletionRoutine( new_irp, TdiNewIrpCompletionHandler, context, TRUE, TRUE, TRUE );

				// Pass the new irp onto the next driver
				return IoCallDriver( connection_manager->next_device, new_irp );
			}
			else
			{
				IoCopyCurrentIrpStackLocationToNext( irp );
				IoSetCompletionRoutine( irp, TdiCompletionHandler, context, TRUE, TRUE, TRUE );
			}
		}
		else
		{
			DbgPrint("Failed to create context - Out of memory?\n");
			IoSkipCurrentIrpStackLocation( irp );
		}
	}
	else
	{
		// Pass the IRP to the target without touching the IRP 
		IoSkipCurrentIrpStackLocation( irp );
	}

	return IoCallDriver(connection_manager->next_device, irp);

}

NTSTATUS TdiCallCompletionRoutine(CompletionRoutine* completion_routine, PDEVICE_OBJECT device_object, PIRP irp)
{	
	UCHAR control = 0;
	NTSTATUS status = STATUS_CONTINUE_COMPLETION;

	if( irp->Cancel )
	{
		control |= SL_INVOKE_ON_CANCEL;
	}
	else
	{
		if( irp->IoStatus.Status >= STATUS_SUCCESS )
		{
			control |= SL_INVOKE_ON_SUCCESS;
		}
		else
		{
			control |= SL_INVOKE_ON_ERROR;
		}
	}

	if( completion_routine->callback )
	{
		if( completion_routine->control & control )
		{
			status = completion_routine->callback( device_object, irp, completion_routine->context );
		}
	}

	return status;
}

NTSTATUS TdiNewIrpCompletionHandler(PDEVICE_OBJECT device_object, PIRP irp, PVOID context)
{
	CompletionRoutineContext* completion_context = (CompletionRoutineContext*)context;

	DbgPrint("TdiNewIrpCompletionHandler - irp %08x new_irp %08x\n", completion_context->original_irp, irp);

	TdiCallCompletionRoutine(&completion_context->completion_routine, device_object, irp);

	completion_context->original_irp->IoStatus = irp->IoStatus;
	completion_context->original_irp->PendingReturned = irp->PendingReturned;
	completion_context->original_irp->MdlAddress = irp->MdlAddress;

	if( irp->PendingReturned )
	{
		IoMarkIrpPending( irp );
		IoMarkIrpPending( completion_context->original_irp );
	}

	IoFreeIrp( irp );

	IoCompleteRequest( completion_context->original_irp, IO_NO_INCREMENT );

	ExFreePoolWithTag( context, CONNECTION_POOL_TAG );

	DbgPrint("TdiNewIrpCompletionHandler END\n");

	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS TdiCompletionHandler(PDEVICE_OBJECT device_object, PIRP irp, PVOID context)
{
	CompletionRoutineContext* completion_context = (CompletionRoutineContext*)context;

	DbgPrint("TdiCompletionHandler START\n");

	TdiCallCompletionRoutine(&completion_context->completion_routine, device_object, irp);

	if( irp->PendingReturned )
	{
		IoMarkIrpPending( irp );   
	}

	ExFreePoolWithTag( context, CONNECTION_POOL_TAG );

	DbgPrint("TdiCompletionHandler END\n");

	return STATUS_CONTINUE_COMPLETION;   
}

// TDI IRP handlers
NTSTATUS TdiDefaultHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	return STATUS_SUCCESS;
}

NTSTATUS TdiAssociateAddressHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	NTSTATUS status;
	ULONG process_id;
	HANDLE local_address_handle = ((TDI_REQUEST_KERNEL_ASSOCIATE *)(&irp_stack->Parameters))->AddressHandle;

	process_id = IoGetRequestorProcessId(irp);

	if( local_address_handle != NULL )
	{
		PFILE_OBJECT local_node = NULL;

		status = ObReferenceObjectByHandle(local_address_handle, GENERIC_READ, NULL, KernelMode, &local_node, NULL);

		if( NT_SUCCESS(status) )
		{
			Connection* connection = CreateConnection(connection_manager, process_id, irp_stack->FileObject, local_node);

			if( connection != NULL )
			{
				DbgPrint("\t\t\t\tInserted: %08x - l=%08x c=%08x p=%i\n", connection, connection->local_node, connection->connection, process_id);

				GetLocalAddress(connection_manager->next_device, connection);
			}

			ObDereferenceObject(local_node);
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS TdiDisassociateAddressHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	//Pair* pair = NULL;
	//Connection* connection = NULL;
	//
	//pair = FindHashMap( &connection_manager->connection_hash_map, (UINT)irp_stack->FileObject);
	//DbgPrint("\t\t\t\tFind: %08x\n", irp_stack->FileObject);
	//if( pair )
	//{
	//	connection = (Connection*)pair->value;
	//	DbgPrint("\t\t\t\tFound: %08x - l=%08x c=%08x p=%i\n", connection, connection->local_node, connection->connection, connection->process_id);
	//}
	//else
	//{
	//	DbgPrint("\t\t\t\tCould not Find: %08x\n", irp_stack->FileObject);
	//}

	//if( connection )
	//{
	//	if( RemoveHashMap( &connection_manager->connection_hash_map, (UINT)irp_stack->FileObject ) )
	//	{
	//		DbgPrint("\t\t\t\tConnection Removed: %08x\n", irp_stack->FileObject);
	//	}
	//	else
	//	{
	//		DbgPrint("\t\t\t\tCould not Find - Remove: %08x\n", irp_stack->FileObject);
	//	}
	//	FreeConnection( connection_manager, connection );
	//}
	RemoveConnection(  connection_manager, irp, irp_stack );
	return STATUS_SUCCESS;
}

NTSTATUS TdiConnectHandlerComplete( PDEVICE_OBJECT device_object, PIRP irp, PVOID context )
{
	//CompletionRoutineContext* completion_context = (CompletionRoutineContext*)context;

	Connection* connection = (Connection*)context;

	DbgPrint("TdiConnectHandlerComplete called\n");

	if( irp->IoStatus.Status == STATUS_SUCCESS )
	{
		connection->state = TCP_STATE_ESTABLISHED;
	}

	return STATUS_SUCCESS;
}

NTSTATUS TdiConnectHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	Pair* pair = NULL;
	Connection* connection = NULL;
	
	pair = FindHashMap( &connection_manager->connection_hash_map, (UINT)irp_stack->FileObject);
	DbgPrint("\t\t\t\tFind: %08x\n", irp_stack->FileObject);
	if( pair )
	{
		connection = (Connection*)pair->value;
		DbgPrint("\t\t\t\tFound: %08x - l=%08x c=%08x p=%i\n", connection, connection->local_node, connection->connection, connection->process_id);
	}
	else
	{
		DbgPrint("\t\t\t\tCould not Find: %08x\n", irp_stack->FileObject);
	}
	
	if( connection )
	{
		PTDI_REQUEST_KERNEL_CONNECT connect_info = NULL;

		connect_info = (PTDI_REQUEST_KERNEL_CONNECT)&irp_stack->Parameters;

		connection->state = TCP_STATE_SYN_SENT;

		if( connection->local_address == NULL )
		{
			GetLocalAddress( connection_manager->next_device, connection );
		}

		if( connect_info )
		{
			TA_ADDRESS *remote_address = NULL;

			remote_address = ((TRANSPORT_ADDRESS*)(connect_info->RequestConnectionInformation->RemoteAddress))->Address;
//PTDI_CONNECTION_INFORMATION
			if(remote_address && remote_address->AddressType == TDI_ADDRESS_TYPE_IP)
			{
			//	TDI_ADDRESS_IP* ip_address = (TDI_ADDRESS_IP*)remote_address ->Address;
				CopyAddress( &connection->remote_address, remote_address  );
			}
		}

		completion_routine->callback = TdiConnectHandlerComplete;
		completion_routine->context = connection;
		completion_routine->control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;
		//IoSetCompletionRoutine(irp, TdiConnectHandlerComplete, connection, TRUE, TRUE, TRUE); 
	}

	

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

NTSTATUS TdiDisconnectHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	TA_ADDRESS *remote_addr;
	PTDI_REQUEST_KERNEL_CONNECT disconnect_info;

	//GetLocalAddress(device_object, pirp_stack->FileObject);


	//DbgBreakPoint();

	disconnect_info = (PTDI_REQUEST_KERNEL_CONNECT)&irp_stack->Parameters;

	if(disconnect_info && 
		disconnect_info->RequestConnectionInformation && 
		disconnect_info->RequestConnectionInformation->RemoteAddress)
	{
		remote_addr = ((TRANSPORT_ADDRESS*)(disconnect_info->RequestConnectionInformation->RemoteAddress))->Address;
		
		if(remote_addr && remote_addr->AddressType == TDI_ADDRESS_TYPE_IP)
		{
			TDI_ADDRESS_IP* ip_address = (TDI_ADDRESS_IP*)remote_addr->Address;
			char *p;
			p = (char *)&ip_address->in_addr;
			DbgPrint("\tDestination: %d.%d.%d.%d:%i ", (UCHAR)p[0], (UCHAR)p[1], (UCHAR)p[2], (UCHAR)p[3], ip_address->sin_port);
		}
	}

	if(disconnect_info && disconnect_info->RequestFlags == TDI_DISCONNECT_ABORT)
		DbgPrint("\tTDI_DISCONNECT_ABORT\n");
	else if(disconnect_info && disconnect_info->RequestFlags == TDI_DISCONNECT_RELEASE)
		DbgPrint("\tTDI_DISCONNECT_RELEASE\n");

	return STATUS_SUCCESS;
}

NTSTATUS TdiSendHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	Pair* pair = NULL;
	Connection* connection = NULL;
	
	pair = FindHashMap( &connection_manager->connection_hash_map, (UINT)irp_stack->FileObject);
	if( pair )
	{
		connection = (Connection*)pair->value;
		DbgPrintConnection( connection );
	}
	else
	{
		DbgPrint("\t\t\t\tCould not Find: %08x\n", irp_stack->FileObject);
	}

	//if( connection->local_address == NULL )

	if( connection )
	{
		DbgPrint("\t\t\t\tGetLocalAddress: %08x %08x\n", irp_stack->FileObject, connection->connection);
		GetLocalAddress( connection_manager->next_device, connection );
	}

	DbgPrintConnection( connection );

	return STATUS_SUCCESS;
}

NTSTATUS TdiReceiveHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	return STATUS_SUCCESS;
}

NTSTATUS TdiSendDatagramHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	return STATUS_SUCCESS;
}

NTSTATUS TdiReceiveDatagramHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	return STATUS_SUCCESS;
}

NTSTATUS TdiSetEventHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{	
	ConnectionContext* connection_context = NULL;
	PTDI_REQUEST_KERNEL_SET_EVENT callback_context = (PTDI_REQUEST_KERNEL_SET_EVENT)&irp_stack->Parameters;

	DbgPrint("\tSET_EVENT_HANDLER = %s\n", EventCallbackNames[callback_context->EventType]);

	if(ConnectionCallbacks[callback_context->EventType])
	{
		if( callback_context->EventHandler == NULL &&
			callback_context->EventContext == NULL )
		{
			Pair* pair = FindHashMap( &connection_manager->callback_hash_map, (UINT)irp_stack->FileObject);
			if( pair )
			{
				connection_context = (ConnectionContext*)pair->value;
				ExFreePoolWithTag( connection_context, CONNECTION_POOL_TAG );
				RemoveHashMap( &connection_manager->callback_hash_map, (UINT)irp_stack->FileObject);
			}
		}
		else
		{
			connection_context = (ConnectionContext*)ExAllocatePoolWithTag(NonPagedPool, sizeof(ConnectionContext), CONNECTION_POOL_TAG);

			if(connection_context)
			{
				connection_context->original_callback = callback_context->EventHandler;
				connection_context->original_context = callback_context->EventContext;
				connection_context->file_object = irp_stack->FileObject;
				connection_context->connection_manager = connection_manager;
				callback_context->EventHandler = ConnectionCallbacks[callback_context->EventType];
				callback_context->EventContext = connection_context;

				InsertHashMap( &connection_manager->callback_hash_map, (UINT)irp_stack->FileObject, connection_context );
			}
		}

	}

	return STATUS_SUCCESS;
}

NTSTATUS TdiQueryInformationHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine )
{
	DbgPrint("TdiQueryInformationHandler = %08x\n", irp_stack->FileObject);
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

VOID UnloadDriver( IN PDRIVER_OBJECT driver_object )
{
	UNICODE_STRING device_name;
	Connection* connection = NULL;
	ConnectionManager* connection_manager = NULL;

	connection_manager = (ConnectionManager*)driver_object->DeviceObject->DeviceExtension;

	DbgPrint("CaptureConnectionMonitor: Unloading\n");

	IoDetachDevice(connection_manager->next_device);

	// Free any trackings to connections that are still open
	KeAcquireSpinLock( &connection_manager->lock, &g_irq_level );

	connection = connection_manager->connection_head;
	while( connection != NULL )
	{
		Connection* next_connection = connection->next;

		FreeConnection( connection_manager, connection );

		connection = next_connection;
	}

	// Destroy the hash map of connections
	CleanupHashMap( &connection_manager->connection_hash_map );
	CleanupHashMap( &connection_manager->callback_hash_map );

	KeReleaseSpinLock( &connection_manager->lock, g_irq_level );

	RtlInitUnicodeString(&device_name, L"\\DosDevices\\CaptureConnectionMonitor");
    IoDeleteSymbolicLink(&device_name);
    if( driver_object->DeviceObject != NULL )
	{
		IoDeleteDevice( driver_object->DeviceObject );
	}

	DbgPrint("CaptureConnectionMonitor: Unloaded\n");
}

// Main entry point into the driver, is called when the driver is loaded
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT driver_object, 
	IN PUNICODE_STRING registry_path
	)
{
	int i = 0;
    NTSTATUS status;
    UNICODE_STRING driver_name;
    UNICODE_STRING device_name;
	UNICODE_STRING attach_device;
	PDEVICE_OBJECT device_object;
    ConnectionManager* connection_manager = NULL;
    
	// Attach to this device
	RtlInitUnicodeString( &attach_device, L"\\Device\\Tcp" );
	// Set the driver name
    RtlInitUnicodeString( &driver_name, L"\\Device\\CaptureConnectionMonitor" );
	// Set the device string
    RtlInitUnicodeString( &device_name, L"\\DosDevices\\CaptureConnectionMonitor");

    // Create and initialize device object
	// Creates the RegistryManager inside the devices extension
	status = IoCreateDevice(
		driver_object,
        sizeof( ConnectionManager ),
        &driver_name,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &device_object
		);
    if(!NT_SUCCESS(status))
	{
		DbgPrint("CaptureConnectionMonitor: ERROR IoCreateDevice ->  \\Device\\CaptureConnectionMonitor - %08x\n", status); 
        return status;
	}
    
	// Get the connection manager from the devices extension
	connection_manager = (ConnectionManager*)device_object->DeviceExtension;

	InitialiseHashMap( &connection_manager->connection_hash_map );
	InitialiseHashMap( &connection_manager->callback_hash_map );
	connection_manager->connection_head = NULL;
	connection_manager->connection_tail = NULL;
	KeInitializeSpinLock( &connection_manager->lock );
	connection_manager->ready = TRUE;

	// Create a symbolic link between the driver and the device
    status = IoCreateSymbolicLink( &device_name, &driver_name );

    if(!NT_SUCCESS(status))
	{
        DbgPrint("CaptureConnectionMonitor: ERROR IoCreateSymbolicLink ->  \\DosDevices\\CaptureConnectionMonitor - %08x\n", status); 
        IoDeleteDevice( device_object );
        return status;
    }

	for(i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		driver_object->MajorFunction[i] = HandleIrp; 
	}
	driver_object->DriverUnload = UnloadDriver;

	status = IoAttachDevice(device_object, &attach_device, &connection_manager->next_device);

	if(!NT_SUCCESS(status))
	{
		IoDeleteSymbolicLink(&device_name);
		IoDeleteDevice( device_object );
		return status;
	}

	DbgPrint("CaptureConnectionMonitor: Successfully Loaded\n");

    return STATUS_SUCCESS;
}

