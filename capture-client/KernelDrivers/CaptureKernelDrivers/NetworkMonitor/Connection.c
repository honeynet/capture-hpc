/*
*	PROJECT: Capture
*	FILE: Connection.c
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

#include "Connection.h"

#include <ntstrsafe.h>

typedef struct _GetAddressContext GetAddressContext;

struct _GetAddressContext
{
	Connection*			connection;
	PFILE_OBJECT		node;
	PTDI_ADDRESS_INFO	address_buffer[TDI_ADDRESS_INFO_MAX];
};

// Got this from the net somewhere
__inline USHORT
ntohs(USHORT x)
{
	return (((x & 0xff) << 8) | ((x & 0xff00) >> 8));
}
	
__inline ULONG
ntohl(ULONG x)
{
	return (((x & 0xffL) << 24) | ((x & 0xff00L) << 8) |
		((x & 0xff0000L) >> 8) | ((x &0xff000000L) >> 24));
}

char* no_address = "0.0.0.0:-1";

VOID InitialiseConnection(Connection* connection)
{

}

VOID DestroyConnection(Connection* connection)
{

}

VOID SetLocalNode(Connection* connection, PFILE_OBJECT local_node)
{
	connection->local_node = local_node;
}

VOID SetRemoteNode(Connection* connection, PFILE_OBJECT remote_node)
{
	connection->remote_node = remote_node;
}

VOID SetNodeAddress(BYTE* address_buffer, PTA_ADDRESS address)
{
	ULONG size = sizeof(TA_ADDRESS) + address->AddressLength;

	RtlCopyMemory(address_buffer, address, size);
}

void VPrintf( char* buffer, int buffer_size, const char* format, ... )
{
	va_list arglist;
	va_start(arglist, format);

	RtlStringCchVPrintfA( buffer, buffer_size, format, arglist);
	
	va_end(arglist);
}

void PrintAddress( PTA_ADDRESS address )
{	
	char buffer[256];

	if( address == NULL )
	{
		RtlStringCchCopyA(buffer, 256, no_address);
	}
	else if(address->AddressType == TDI_ADDRESS_TYPE_IP)
	{
		TDI_ADDRESS_IP* ip_address = (TDI_ADDRESS_IP*)address->Address;
		char *p;
		p = (char *)&ip_address->in_addr;

		VPrintf( buffer, 256, "%d.%d.%d.%d:%u", (UCHAR)p[0], (UCHAR)p[1], (UCHAR)p[2], (UCHAR)p[3], ntohs(ip_address->sin_port));
	}

	DbgPrint("\t\t\t\t\t\t\tAddress = %s\n", buffer);
}

VOID AddressToString(PTA_ADDRESS address, char* buffer, unsigned long buffer_size)
{
	if( address == NULL )
	{
		RtlStringCchCopyA( buffer, buffer_size, no_address );
	}
	else if(address->AddressType == TDI_ADDRESS_TYPE_IP)
	{
		TDI_ADDRESS_IP* ip_address = (TDI_ADDRESS_IP*)address->Address;
		char *p;
		p = (char *)&ip_address->in_addr;

		VPrintf( buffer, buffer_size, "%d.%d.%d.%d:%u", (UCHAR)p[0], (UCHAR)p[1], (UCHAR)p[2], (UCHAR)p[3], ntohs(ip_address->sin_port));
	}
}

VOID SetLocalNodeAddress(Connection* connection, PTA_ADDRESS address)
{
	SetNodeAddress(connection->local_node_address, address);
}

VOID SetRemoteNodeAddress(Connection* connection, PTA_ADDRESS address)
{
	PrintAddress( address );
	SetNodeAddress(connection->remote_node_address, address);
}

NTSTATUS GetLocalAddressComplete( PDEVICE_OBJECT device_object, PIRP irp, PVOID context )
{
	GetAddressContext* address_context = (GetAddressContext*)context;

	if( irp->IoStatus.Status == STATUS_SUCCESS )
	{
		SetLocalNodeAddress(address_context->connection, &((PTDI_ADDRESS_INFO)address_context->address_buffer)->Address.Address[0]);
		PrintAddress((PTA_ADDRESS)address_context->connection->local_node_address);
	}
	
	ExFreePoolWithTag(address_context, CONNECTION_POOL_TAG);

	if( irp->MdlAddress )
	{
		IoFreeMdl( irp->MdlAddress );
	}

	IoFreeIrp(irp);

	return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID GetLocalAddress(Connection* connection, PDEVICE_OBJECT device_object)
{
	NTSTATUS status;
	PMDL mdl = NULL;
	PIRP query_irp = NULL;
	KEVENT irp_complete_event;
	BOOLEAN immediate_mode = FALSE;
	IO_STATUS_BLOCK io_status_block;
	GetAddressContext* address_context = NULL;

	address_context = (GetAddressContext*)ExAllocatePoolWithTag(NonPagedPool, sizeof(GetAddressContext), CONNECTION_POOL_TAG);

	if( address_context == NULL )
		return;

	address_context->connection = connection;
	address_context->node = connection->local_node;

	// NOTE: Can't use TdiBuildInternalDeviceControlIrp as sometimes we are at IRQL = DISPATCH_LEVEL
	// We always got an access violation when coming from netbt which looks like it is being run in a timer DPC
	//query_irp = TdiBuildInternalDeviceControlIrp( TDI_QUERY_INFORMATION, device_object, connection->local_node, NULL, &io_status_block );
	query_irp = IoAllocateIrp(device_object->StackSize + 1, FALSE);

	if( query_irp == NULL )
		return;

	if(KeGetCurrentIrql() <= DISPATCH_LEVEL)
	{
		KeInitializeEvent(&irp_complete_event, NotificationEvent, FALSE);

		query_irp->UserEvent = &irp_complete_event;
		query_irp->UserIosb = &io_status_block;

		immediate_mode = TRUE;
	}

	mdl = IoAllocateMdl( &address_context->address_buffer, TDI_ADDRESS_INFO_MAX, FALSE, FALSE, NULL );

	if( mdl != NULL )
	{
		MmBuildMdlForNonPagedPool(mdl);

		TdiBuildQueryInformation( query_irp, device_object, connection->local_node, GetLocalAddressComplete, address_context, TDI_QUERY_ADDRESS_INFO, mdl);   

		status = IoCallDriver( device_object, query_irp ); 

		if(immediate_mode == TRUE && status == STATUS_PENDING)
		{
			DbgPrint("GetLocalAdress: Waiting for IRP\n");
			// Wait for the IRP to finish. This should call our completion handler as well
			KeWaitForSingleObject((PVOID)&irp_complete_event, Executive, KernelMode, TRUE, NULL); 
			status = io_status_block.Status; 
		}

		return;
	}

	if(mdl)			IoFreeMdl(mdl);
	if(query_irp)	IoFreeIrp(query_irp);

	return;
}