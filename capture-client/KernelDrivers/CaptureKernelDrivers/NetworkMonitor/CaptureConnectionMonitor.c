/*
 *	PROJECT: Capture
 *	FILE: CaptureConnectionMonitor.c
 *	AUTHORS:	Ramon Steenson (rsteenson@gmail.com)
 *				Christian Seifert (christian.seifert@gmail.com)
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

#include "CaptureTdiIrpHandlers.h"
#include "CaptureConnectionMonitor.h"

#include <strsafe.h>

KIRQL g_irq_level;

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

// Main entry point into the driver, is called when the driver is loaded
NTSTATUS DriverEntry(
					 IN PDRIVER_OBJECT driver_object, 
					 IN PUNICODE_STRING registry_path
					 )
{
	int i = 0;
	NTSTATUS status;
	UNICODE_STRING tcpdriver_name;
	UNICODE_STRING tcpdevice_name;
	UNICODE_STRING udpdriver_name;
	UNICODE_STRING udpdevice_name;

	UNICODE_STRING attach_device;
	PDEVICE_OBJECT tcpdevice_object;
	PDEVICE_OBJECT udpdevice_object;

	ConnectionManager* connection_manager = NULL;


	//Set irp handles for both tcp and udp
	for(i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		driver_object->MajorFunction[i] = HandleIrp; 
	}
	driver_object->DriverUnload = UnloadDriver;


	// CREATE AND INITIALIZE TCP DEVICE OBJECT///////
	

	//Attach to tcp device
	RtlInitUnicodeString( &attach_device, L"\\Device\\Tcp" );

	// Set the driver name
	RtlInitUnicodeString( &tcpdriver_name, L"\\Device\\TCPCaptureConnectionMonitor" );
	// Set the device string
	RtlInitUnicodeString( &tcpdevice_name, L"\\DosDevices\\TCPCaptureConnectionMonitor");

	status = IoCreateDevice(
		driver_object,
		sizeof( ConnectionManager ),
		&tcpdriver_name,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&tcpdevice_object
		);

	if(!NT_SUCCESS(status))
	{
		DbgPrint("CaptureConnectionMonitor: ERROR IoCreateDevice ->  \\Device\\TCPCaptureConnectionMonitor - %08x\n", status);
		return status;
	}

	// Get the connection manager from the devices extension
	connection_manager = (ConnectionManager*)tcpdevice_object->DeviceExtension;

	connection_manager->device = tcpdevice_object;
	connection_manager->device_type=TCP_DEVICE;

	status = IoAttachDevice(tcpdevice_object, &attach_device, &connection_manager->next_device);

	InitialiseConnectionManager(connection_manager);
	if(!NT_SUCCESS(status))
	{
		DbgPrint("CaptureConnectionMonitor: Can't attach TCP device - %08x\n", status);
		IoDeleteDevice( tcpdevice_object );
		return status;
	}
	// Create a symbolic link between the driver and the device
	status = IoCreateSymbolicLink( &tcpdevice_name, &tcpdriver_name );

	if(!NT_SUCCESS(status))
	{
		DbgPrint("CaptureConnectionMonitor: ERROR IoCreateSymbolicLink ->  \\DosDevices\\TCPCaptureConnectionMonitor - %08x\n", status); 
		IoDeleteDevice( tcpdevice_object );
		return status;
	}
	DbgPrint("TCPCaptureConnectionMonitor: Successfully Loaded\n");


	// CREATE AND INITIALIZE UDP DEVICE OBJECT///////
	
	//Attach to tcp device
	RtlInitUnicodeString( &attach_device, L"\\Device\\Udp" );

	// Set the driver name
	RtlInitUnicodeString( &udpdriver_name, L"\\Device\\UDPCaptureConnectionMonitor" );
	// Set the device string
	RtlInitUnicodeString( &udpdevice_name, L"\\DosDevices\\UDPCaptureConnectionMonitor");

	status = IoCreateDevice(
		driver_object,
		sizeof( ConnectionManager ),
		&udpdriver_name,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&udpdevice_object
		);

	if(!NT_SUCCESS(status))
	{
		DbgPrint("CaptureConnectionMonitor: ERROR IoCreateDevice ->  \\Device\\UDPCaptureConnectionMonitor - %08x\n", status);
		return status;
	}

	// Get the connection manager from the devices extension
	connection_manager = (ConnectionManager*)udpdevice_object->DeviceExtension;

	connection_manager->device = udpdevice_object;
	connection_manager->device_type=UDP_DEVICE;

	status = IoAttachDevice(udpdevice_object, &attach_device, &connection_manager->next_device);

	InitialiseConnectionManager(connection_manager);
	if(!NT_SUCCESS(status))
	{
		DbgPrint("CaptureConnectionMonitor: Can't attach UDP device - %08x\n", status);
		IoDeleteDevice( udpdevice_object );
		IoDeleteSymbolicLink(&tcpdevice_name);
		IoDeleteDevice(tcpdevice_object);
		return status;
	}
	// Create a symbolic link between the driver and the device
	status = IoCreateSymbolicLink( &udpdevice_name, &udpdriver_name );

	if(!NT_SUCCESS(status))
	{
		DbgPrint("CaptureConnectionMonitor: ERROR IoCreateSymbolicLink ->  \\DosDevices\\UDPCaptureConnectionMonitor - %08x\n", status); 
		IoDeleteDevice( udpdevice_object );
		IoDeleteSymbolicLink(&tcpdevice_name);
		IoDeleteDevice(tcpdevice_object);
		return status;
	}

	DbgPrint("UDPCaptureConnectionMonitor: Successfully Loaded\n");

	return STATUS_SUCCESS;
}

VOID UnloadDriver( IN PDRIVER_OBJECT driver_object )
{
	UNICODE_STRING device_name;
	Connection* connection = NULL;
	ConnectionManager* connection_manager = NULL;

	connection_manager = (ConnectionManager*)driver_object->DeviceObject->DeviceExtension;
	if (connection_manager->device_type==TCP_DEVICE)
		RtlInitUnicodeString(&device_name, L"\\DosDevices\\TCPCaptureConnectionMonitor");
	else RtlInitUnicodeString(&device_name, L"\\DosDevices\\UDPCaptureConnectionMonitor");

	DbgPrint("CaptureConnectionMonitor: Unloading %s\n", device_name);

	IoDetachDevice(connection_manager->next_device);

	DestroyConnectionManager(connection_manager);

	IoDeleteSymbolicLink(&device_name);
	if( driver_object->DeviceObject != NULL )
	{
		IoDeleteDevice( driver_object->DeviceObject );
	}

	DbgPrint("CaptureConnectionMonitor: Unloaded %s\n", device_name);
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
	if (connection_manager->device_type==TCP_DEVICE)
		DbgPrint("TCP DEVICE\t\t");
	else if (connection_manager->device_type==UDP_DEVICE)
		DbgPrint("UDP DEVICE\t\t");

	if(pirp_stack->MajorFunction == IRP_MJ_DEVICE_CONTROL)
	{
		if (KeGetCurrentIrql() == PASSIVE_LEVEL)
		{
			status = TdiMapUserRequest(device_object, irp, pirp_stack);
			
			if(status == STATUS_SUCCESS)
			{
				DbgPrint("TDI MapUserRequest Succeeded\n");
			}
		}

		if(pirp_stack->Parameters.DeviceIoControl.IoControlCode == GET_SHARED_EVENT_LIST_IOCTL)
		{
			GetSharedEventListIoctlHandler(&connection_manager->event_list, device_object, irp, pirp_stack);
			DbgPrint("TDI IRP_MJ_DEVICE_CONTROL: GET_SHARED_EVENT_LIST_IOCTL\t\t%08x\n", pirp_stack->FileObject);
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_SUCCESS;
		}

		//else if (pirp_stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_TCP_QUERY_INFORMATION_EX)
		//{
		//	DbgPrint("TDI IRP_MJ_DEVICE_CONTROL: IOCTL_TCP_QUERY_INFORMATION_EX\t\t%08x\n", pirp_stack->FileObject);
		//}
		//else if (pirp_stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_TCP_SET_INFORMATION_EX)
		//{
		//	DbgPrint("TDI IRP_MJ_DEVICE_CONTROL: IOCTL_TCP_SET_INFORMATION_EX\t\t%08x\n", pirp_stack->FileObject);
		//}
		else if(pirp_stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER)
		{
			DbgPrint("TDI IRP_MJ_DEVICE_CONTROL: IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER\n");
		}
		else DbgPrint("TDI IRP_MJ_DEVICE_CONTROL\t\t%08x\n", pirp_stack->FileObject);
	}

	if(pirp_stack->MajorFunction == IRP_MJ_CREATE )
	{
		DbgPrint("TDI IRP = IRP_MJ_CREATE\t\t%08x\n", pirp_stack->FileObject);	

		TdiCreateHandler( connection_manager, irp, pirp_stack, &completion_routine );
	}

	else if(pirp_stack->MajorFunction == IRP_MJ_CLOSE )
	{
		DbgPrint("TDI IRP = IRP_MJ_CLOSE\t\t%08x\n", pirp_stack->FileObject);	

		TdiCloseHandler( connection_manager, irp, pirp_stack, &completion_routine );
	}

	else if(pirp_stack->MajorFunction == IRP_MJ_CLEANUP )
	{
		DbgPrint("TDI IRP = IRP_MJ_CLEANUP\t\t%08x\n", pirp_stack->FileObject);	

		TdiCleanupHandler( connection_manager, irp, pirp_stack, &completion_routine );
	}

	else if(pirp_stack->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL)
	{
		int tdi_irp = pirp_stack->MinorFunction;

		DbgPrint("TDI IRP = %s\t\t%08x\n", TdiIrpNames[pirp_stack->MinorFunction], pirp_stack->FileObject);	

		if(( tdi_irp < num_tdi_irp_handlers ) && (((int)pirp_stack->FileObject->FsContext2==TDI_CONNECTION_FILE)||((int)pirp_stack->FileObject->FsContext2==TDI_TRANSPORT_ADDRESS_FILE)))
		{
			((TdiIrpHandler)(tdi_irp_handlers[tdi_irp]))( connection_manager, irp, pirp_stack, &completion_routine );
		}
		else DbgPrint("Don't know IRP_MJ_INTERNAL_DEVICE_CONTROL");
	}


	// Setup the completion routine and pass the IRP down the stack
	if( completion_routine.callback )
	{
		CompletionRoutineContext* context = (CompletionRoutineContext*)ExAllocatePoolWithTag( NonPagedPool, sizeof(CompletionRoutineContext), CONNECTION_POOL_TAG );

		if( context != NULL )
		{
			RtlZeroMemory( context, sizeof(CompletionRoutineContext) );

			context->completion_routine.callback = completion_routine.callback;
			context->completion_routine.context = completion_routine.context;
			context->completion_routine.control = completion_routine.control;

			if( irp->CurrentLocation <= 1 )
			{
				// Not enough stack locations exists to put our completion routine in
				// We create a new irp with enough locations and send that down to the
				// next lowest driver. In the completion handler we call the callback,
				// free the irp we created and complete the original irp which calls
				// its completion routine.
				// We do this because drivers like netbt use legacy code that don't
				// have any free stack locations
				return SetupNewIrpCompletionRoutineCallDriver(connection_manager, device_object, irp, pirp_stack, context);
			}
			else
			{
				// Setup the completion routine and pass the original IRP
				return SetupCompletionRoutineCallDriver(connection_manager, irp, pirp_stack, context);
			}
		}
	}

	// Pass the IRP to the target without touching the IRP 
	IoSkipCurrentIrpStackLocation( irp );

	// Pass the original IRP
	return IoCallDriver(connection_manager->next_device, irp);
}

/// Sets up a new IRP with the same parameters as the original but with enough stack locations
/// to store our completion routine. It will pass this new irp down the stack. Once the new
/// IRP is completed the original will be marked as completed in TdiNewIrpCompletionHandler
/// which will call the original completion routines. See comment in HandleIrp for more info
/// @param	connection_manager	The connection manager for this kernel driver
/// @param	device_object		This device
/// @param	irp					The current irp
/// @param	irp_stack			Our irp stack location
/// @param	context				The completion routine context containing our callback and context
NTSTATUS SetupNewIrpCompletionRoutineCallDriver(ConnectionManager* connection_manager, 
												PDEVICE_OBJECT device_object,
												PIRP irp, 
												PIO_STACK_LOCATION irp_stack, 
												CompletionRoutineContext* context)
{
	PIRP new_irp = NULL;
	PIO_STACK_LOCATION new_sirp = NULL;	

	// Create a new IRP with enough stack locations
	new_irp = IoAllocateIrp(connection_manager->next_device->StackSize + 1, FALSE);

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

	// Make the stack location valid
	IoSetNextIrpStackLocation(new_irp);

	// Set our device so that we receive a pointer to it in our completion routine
	IoGetCurrentIrpStackLocation(new_irp)->DeviceObject = device_object;

	IoSetCompletionRoutine( new_irp, TdiNewIrpCompletionHandler, context, TRUE, TRUE, TRUE );

	// Give our new irp the same stack location as the original
	new_sirp = IoGetNextIrpStackLocation(new_irp);
	RtlCopyMemory( new_sirp, irp_stack, FIELD_OFFSET(IO_STACK_LOCATION, CompletionRoutine));

	// Pass the new irp onto the next driver
	return IoCallDriver( connection_manager->next_device, new_irp );
}

/// Sets up the completion routine in the original irp
/// Will call TdiCallCompletionRoutine when the irp completes which will then call
/// our completion routine stored in the context
/// @param	connection_manager	The connection manager for this kernel driver
/// @param	irp					The current irp
/// @param	irp_stack			Our irp stack location
/// @param	context				The completion routine context containing our callback and context
NTSTATUS SetupCompletionRoutineCallDriver(ConnectionManager* connection_manager, 
										  PIRP irp, 
										  PIO_STACK_LOCATION irp_stack, 
										  CompletionRoutineContext* context)
{
	IoCopyCurrentIrpStackLocationToNext( irp );
	IoSetCompletionRoutine( irp, TdiCompletionHandler, context, TRUE, TRUE, TRUE );

	return IoCallDriver( connection_manager->next_device, irp );
}

/// A small wrapper that handles the calling of completion routines we set for IRPs
/// Will call the completion routine stored inside CompletionRoutine only if the status
/// matches what the IRP returned
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

/// Called when a IRP created in our driver has been completed and we set a completion routine.
/// Only called when an IRP does not have enough stack locations to hold our completion routine
/// A new IRP is created and this is called when that is finished. It will call our completion
/// routine, free our IRP and complete the original IRP, which will in turn call its
/// completion routines
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

/// Called when a IRP created in our driver has been completed and we set a completion routine.
/// This is called most times as most IRPs have enough stack locations to insert our
/// completion routine. This will call our completion routine.
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
