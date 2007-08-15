/*
 *	PROJECT: Capture
 *	FILE: CaptureRegistryMonitor.c
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
#include <ntifs.h>
#include <ntstrsafe.h>

#define FILE_DEVICE_UNKNOWN             0x00000022
#define IOCTL_UNKNOWN_BASE              FILE_DEVICE_UNKNOWN
#define IOCTL_CAPTURE_GET_REGEVENTS	  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_NEITHER,FILE_READ_DATA | FILE_WRITE_DATA) 

#define IOCTL_CAPTURE_START    CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0805, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_CAPTURE_STOP    CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0806, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define REGISTRY_PACKET_POOL_TAG 'pRE'

typedef unsigned int UINT;
typedef char * PCHAR;
typedef PVOID           POBJECT;

/* Registry event */
typedef struct  _REGISTRY_EVENT {
	REG_NOTIFY_CLASS type;
	TIME_FIELDS time;
	WCHAR name[1024];
	HANDLE processID;
} REGISTRY_EVENT, * PREGISTRY_EVENT;

/* Storage for registry event to be put into a linked list */
typedef struct  _REGISTRY_EVENT_PACKET {
    LIST_ENTRY     Link;
	REGISTRY_EVENT event;

} REGISTRY_EVENT_PACKET, * PREGISTRY_EVENT_PACKET; 

/* Context stuff */
typedef struct _DEVICE_EXTENSION 
{
    PDEVICE_OBJECT DeviceObject;
	FAST_MUTEX FastLock;
	LIST_ENTRY lRegistry;
	KSPIN_LOCK  lRegistrySpinLock;
	UINT lSize;
	UINT count;
	BOOLEAN bRunning;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/* Methods */
NTSTATUS KDispatchCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS KDispatchIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

void UnloadDriver(PDRIVER_OBJECT DriverObject);

NTSTATUS RegistryCallback(IN PVOID CallbackContext, IN PVOID  Argument1, IN PVOID  Argument2);

NTSTATUS HandleIoctlGetRegEvents(IN PDEVICE_OBJECT DeviceObject,PIRP Irp, PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten);

BOOLEAN GetObjectCompleteName(IN PVOID Object, IN PUNICODE_STRING PartialName, PREGISTRY_EVENT_PACKET rEventPacket, PDEVICE_EXTENSION extension);
BOOLEAN InsertIntoLinkedList(PREGISTRY_EVENT_PACKET rEventPacket, PDEVICE_EXTENSION extension);

/* Global device object so our process callback can use the information*/
// Shouldn't really be needed as most callbacks we use can pass in a context value
PDEVICE_OBJECT g_pDeviceObject;

// Cookie from the registration of the registry callback
LARGE_INTEGER  Cookie;

/*	Main entry point into the driver, is called when the driver is loaded */
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject, 
	IN PUNICODE_STRING RegistryPath
	)
{
    NTSTATUS        ntStatus;
    UNICODE_STRING  uszDriverString;
    UNICODE_STRING  uszDeviceString;
	UNICODE_STRING	uszFilterDriver;

    PDEVICE_OBJECT    pDeviceObject;
    PDEVICE_EXTENSION extension;
    
	// Point uszDriverString at the driver name
    RtlInitUnicodeString(&uszDriverString, L"\\Device\\CaptureRegistryMonitor");

    // Create and initialize device object
    ntStatus = IoCreateDevice(
		DriverObject,
        sizeof(DEVICE_EXTENSION),
        &uszDriverString,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &pDeviceObject
		);
    if(ntStatus != STATUS_SUCCESS)
        return ntStatus;
    
	// Assign extension variable
    extension = pDeviceObject->DeviceExtension;
    
	// Point uszDeviceString at the device name
    RtlInitUnicodeString(&uszDeviceString, L"\\DosDevices\\CaptureRegistryMonitor");
    
	// Create symbolic link to the user-visible name
    ntStatus = IoCreateSymbolicLink(&uszDeviceString, &uszDriverString);

    if(ntStatus != STATUS_SUCCESS)
    {
        // Delete device object if not successful
        IoDeleteDevice(pDeviceObject);
        return ntStatus;
    }

	extension->lSize = 0;
	extension->count = 0;
	extension->bRunning = FALSE;
	ExInitializeFastMutex(&extension->FastLock);
	KeInitializeSpinLock(&extension->lRegistrySpinLock);
	InitializeListHead(&extension->lRegistry);

    // Assign global pointer to the device object for use by the callback functions
    g_pDeviceObject = pDeviceObject;

    // Load structure to point to IRP handlers
    DriverObject->DriverUnload                         = UnloadDriver;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = KDispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = KDispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KDispatchIoctl;

	//CmRegisterCallback(RegistryCallback, extension, &Cookie);

    // Return success
    return ntStatus;
}

NTSTATUS KDispatchCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information=0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

// Inserts a registry packet containing a registry event into the linked list.
// Checks the size of the linked list so that we don't use up all of the kernel space memory
BOOLEAN InsertIntoLinkedList(PREGISTRY_EVENT_PACKET rEventPacket, PDEVICE_EXTENSION extension)
{
	if(extension->lSize <= 10000) {
		ExInterlockedInsertTailList(&extension->lRegistry,&rEventPacket->Link,&extension->lRegistrySpinLock);
		extension->lSize++;
		return TRUE;
	} else {
		DbgPrint("\t\tInsufficient Space in Linked List\n");
	}
	return FALSE;
}

// GetObjectCompleteName
// Gets the full path (complete) name of an object. Very nasty API, as a lot of checks must be
// done to insure that the object is valid.
BOOLEAN GetObjectCompleteName(IN PVOID Object, IN PUNICODE_STRING PartialName, PREGISTRY_EVENT_PACKET rEventPacket, PDEVICE_EXTENSION extension)
{
	BOOLEAN set = FALSE;
	if( rEventPacket != NULL)
	{
		UINT returnedLength;

		/* TODO bad ... and ugly ... find a better way to determine whether the path is already complete */
		// Check if it is already complete else use ObQueryNameString to attempt to get the complete name
		if( (PartialName->Buffer != NULL) && (((PartialName->Buffer[0] == '\\') || (PartialName->Buffer[0] == '%')) ||
			((PartialName->Buffer[0] == 'T') && (PartialName->Buffer[1] == 'R') && (PartialName->Buffer[2] == 'Y') && (PartialName->Buffer[3] == '\\'))) )
		{
			//DbgPrint("TRY: %i -> %wZ\n", PartialName->Length,PartialName);
			RtlCopyMemory(rEventPacket->event.name, PartialName->Buffer, PartialName->Length);
			set = TRUE;
		} else {
			if((MmIsAddressValid(Object) == TRUE) && (Object > 0))
			{
				PUNICODE_STRING	storageDriverName;
				
				storageDriverName = ExAllocatePoolWithTag(NonPagedPool, 1024*sizeof(UNICODE_STRING), REGISTRY_PACKET_POOL_TAG); 
				if(storageDriverName != NULL)
				{
					NTSTATUS status;
				
					status = ObQueryNameString( Object,(POBJECT_NAME_INFORMATION)storageDriverName,1024,&returnedLength );
					if( NT_SUCCESS (status))
					{
						//DbgPrint("EXISTS: %i -> %wZ\n", storageDriverName->Length,storageDriverName);
						RtlCopyMemory(rEventPacket->event.name, storageDriverName->Buffer, storageDriverName->Length);
						set = TRUE;
					} else {
						//DbgPrint("FAIL1: %i -> %wZ\n", PartialName->Length,PartialName);
					}
					ExFreePoolWithTag ( storageDriverName, REGISTRY_PACKET_POOL_TAG );
				}
			} else {
						//DbgPrint("FAIL2: %i -> %wZ\n", PartialName->Length,PartialName);
			}
		}
	}
	return set;
}

/* Registry Callback Routing */
// Gets called every time a registry event occurs in the system. Mostly compatible with Windows XP.
// Vista contains a better API and more information can be gathered from the structures passed to it.
// 
// NOTE This function will not monitor events that have not succeeded for instance an open registry event
// that occurs on a non-existant registry key. However some information can be gathered from the partial
// name that some information objects pass as they are sometimes complete names ... most of the time they
// are not and henceforth are not reliable.
//
// WARNING this function can be called 10,000 times in a second or two. Make sure that you
// are freeing the memory you are allocating else you will lock the system up and or get an unreliable system
// (my favourite is the internet will stop working on the system) and your laptop will set on fire.
NTSTATUS RegistryCallback(IN PVOID CallbackContext, IN PVOID  Argument1, IN PVOID  Argument2)
{	
	REG_NOTIFY_CLASS Type;
	LARGE_INTEGER CurrentSystemTime;
	LARGE_INTEGER CurrentLocalTime;
	TIME_FIELDS TimeFields;
	BOOLEAN valid = FALSE;
	UNICODE_STRING unknownName;
	UNICODE_STRING typeName;
	
	PDEVICE_EXTENSION extension = (PDEVICE_EXTENSION)CallbackContext;

	ExAcquireFastMutex(&extension->FastLock);
	RtlInitUnicodeString(&unknownName,L"???");


	
	Type = (REG_NOTIFY_CLASS)Argument1;
	try
	{
		REGISTRY_EVENT_PACKET event;
		PREGISTRY_EVENT_PACKET rEventPacket;
		RtlZeroMemory(&event, sizeof(REGISTRY_EVENT_PACKET));

		rEventPacket = &event;

		KeQuerySystemTime(&CurrentSystemTime);
		ExSystemTimeToLocalTime(&CurrentSystemTime,&CurrentLocalTime);
		RtlTimeToTimeFields(&CurrentLocalTime,&TimeFields);
		RtlCopyMemory(&rEventPacket->event.time, &TimeFields, sizeof(TIME_FIELDS));

		/* TODO change this to a switch statement */
		// If else statement which handles the different types of information structures passed
		// to it from the registry manager. At the moment most are post registry calls i.e. they
		// have already been completed. 
		if(Type == RegNtPostCreateKey)
		{		
			PREG_POST_CREATE_KEY_INFORMATION create = (PREG_POST_CREATE_KEY_INFORMATION)Argument2;
			PVOID* po;
			po = create->Object;
			RtlInitUnicodeString(&typeName,L"RegNtPostCreateKey");
			// *po == 0 the CompleteName could actually be complete and not a partial path
			// We should check later but initial tests show that it is always correct.
			// (create->Status >= 0) This is a post event ... if the event didn't didn't succeed
			// Then the object will be invalid ... however the above may hold the complete name

			rEventPacket->event.type = RegNtPostCreateKey;
			if(NT_SUCCESS(create->Status)) {
				valid = GetObjectCompleteName(*po,create->CompleteName, rEventPacket, extension);
			} else {
				//DbgPrint("RegNtPostOpenKey: KEY NOT FOUND -- %ws\n", open->CompleteName->Buffer);
			}
		} else if(Type == RegNtPostOpenKey) {		
			PREG_POST_OPEN_KEY_INFORMATION open = (PREG_POST_OPEN_KEY_INFORMATION)Argument2;
			PVOID* po;
			po = open->Object;

			RtlInitUnicodeString(&typeName,L"RegNtPostOpenKey");	

			rEventPacket->event.type = RegNtPostOpenKey;
			if(NT_SUCCESS(open->Status)) {
				valid = GetObjectCompleteName(*po,open->CompleteName, rEventPacket, extension);
			} else {
				//DbgPrint("RegNtPostOpenKey: KEY NOT FOUND -- %ws\n", open->CompleteName->Buffer);
			}
		} else if(Type == RegNtPreDeleteKey) {
			PREG_DELETE_KEY_INFORMATION del = (PREG_DELETE_KEY_INFORMATION)Argument2;
			PVOID* po;
			po = del->Object;

			RtlInitUnicodeString(&typeName,L"RegNtPreDeleteKey");	

			valid = GetObjectCompleteName(*po,&unknownName, rEventPacket, extension);
			rEventPacket->event.type = RegNtPreDeleteKey;
		
		} else if(Type == RegNtDeleteValueKey) {
			PREG_DELETE_VALUE_KEY_INFORMATION del = (PREG_DELETE_VALUE_KEY_INFORMATION)Argument2;
			PVOID* po;
			po = del->Object;

			RtlInitUnicodeString(&typeName,L"RegNtDeleteValueKey");	
				
			valid = GetObjectCompleteName(del->Object,del->ValueName, rEventPacket, extension);
			if(del->ValueName->Length > 0) 
			{
				RtlStringCbCatW(rEventPacket->event.name, 1024, L"\\");
				RtlStringCbCatW(rEventPacket->event.name, 1024, del->ValueName->Buffer);
			}
			rEventPacket->event.type = RegNtDeleteValueKey;
			
		} else if(Type == RegNtPreSetValueKey) {
			PREG_SET_VALUE_KEY_INFORMATION set = (PREG_SET_VALUE_KEY_INFORMATION)Argument2;
			PVOID* po;
			po = set->Object;

			RtlInitUnicodeString(&typeName,L"RegNtPreSetValueKey");	
			//DbgPrint("RegNtPreSetValueKey\n");
			valid = GetObjectCompleteName(set->Object,set->ValueName, rEventPacket, extension);
			if(set->ValueName->Length > 0) 
			{
				RtlStringCbCatW(rEventPacket->event.name, 1024, L"\\");
				//DbgPrint("Appening: size: %i -> %wZ\n", set->ValueName->Length, set->ValueName);
				RtlStringCbCatNW(rEventPacket->event.name, 1024,set->ValueName->Buffer,set->ValueName->Length);
			}
			rEventPacket->event.type = RegNtPreSetValueKey;	
		} else if(Type == RegNtEnumerateKey) {
			PREG_ENUMERATE_KEY_INFORMATION enumerate = (PREG_ENUMERATE_KEY_INFORMATION)Argument2;
			PVOID* po;
			po = enumerate->Object;

			RtlInitUnicodeString(&typeName,L"RegNtEnumerateKey");

			valid = GetObjectCompleteName(enumerate->Object, &unknownName, rEventPacket, extension);
			rEventPacket->event.type = RegNtEnumerateKey;
		} else if(Type == RegNtEnumerateValueKey) {
			PREG_ENUMERATE_VALUE_KEY_INFORMATION enumerate = (PREG_ENUMERATE_VALUE_KEY_INFORMATION)Argument2;
			PVOID* po;
			po = enumerate->Object;

			RtlInitUnicodeString(&typeName,L"RegNtEnumerateValueKey");
		
			valid = GetObjectCompleteName(enumerate->Object, &unknownName, rEventPacket, extension);
			rEventPacket->event.type = RegNtEnumerateValueKey;	
		} else if(Type == RegNtQueryKey) {
			PREG_QUERY_KEY_INFORMATION enumerate = (PREG_QUERY_KEY_INFORMATION)Argument2;
			PVOID* po;
			po = enumerate->Object;

			RtlInitUnicodeString(&typeName,L"RegNtQueryKey");

			rEventPacket->event.type = RegNtQueryKey;
			valid = GetObjectCompleteName(enumerate->Object, &unknownName, rEventPacket, extension);		
		} else if(Type == RegNtQueryValueKey) {
			PREG_QUERY_VALUE_KEY_INFORMATION enumerate = (PREG_QUERY_VALUE_KEY_INFORMATION)Argument2;
			PVOID* po;
			po = enumerate->Object;

			RtlInitUnicodeString(&typeName,L"RegNtQueryValueKey");
	
			rEventPacket->event.type = RegNtQueryValueKey;
			valid = GetObjectCompleteName(enumerate->Object, enumerate->ValueName, rEventPacket, extension);
			//rEventPacket->event.name
			if(enumerate->ValueName->Length > 0) 
			{
				RtlStringCbCatW(rEventPacket->event.name, 1024, L"\\");
				RtlStringCbCatNW(rEventPacket->event.name, 1024,enumerate->ValueName->Buffer, enumerate->ValueName->Length);
			}

		} else if(Type == RegNtKeyHandleClose) {
			PREG_KEY_HANDLE_CLOSE_INFORMATION enumerate = (PREG_KEY_HANDLE_CLOSE_INFORMATION)Argument2;
			PVOID* po;
			po = enumerate->Object;

			RtlInitUnicodeString(&typeName,L"RegNtKeyHandleClose");		

			rEventPacket->event.type = RegNtKeyHandleClose;
			valid = GetObjectCompleteName(enumerate->Object, &unknownName, rEventPacket, extension);	
		} else {
			RtlInitUnicodeString(&typeName,L"???");
		}

		if(valid)
		{
			// NOTE for kernel development I found it easier to only allocate memory when we are absolutely
			// sure we have got valid data. Makes it much easier to debug as we know it will always succeed and
			// won't have a kernel memory leak.
			PREGISTRY_EVENT_PACKET e = ExAllocatePoolWithTag(NonPagedPool, sizeof(REGISTRY_EVENT_PACKET), REGISTRY_PACKET_POOL_TAG);
			rEventPacket->event.processID = PsGetCurrentProcessId();
			RtlCopyMemory(e, rEventPacket, sizeof(REGISTRY_EVENT_PACKET));
			extension->count++;
			if(!InsertIntoLinkedList(e, extension))
			{
				ExFreePoolWithTag ( e, REGISTRY_PACKET_POOL_TAG );
			}
		}
	} except( EXCEPTION_EXECUTE_HANDLER ) {
		DbgPrint("%ws: EXCEPTION!!!!!!\n", typeName.Buffer);
	}

	ExReleaseFastMutex(&extension->FastLock);
	
	return STATUS_SUCCESS;
}

NTSTATUS KDispatchIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS              ntStatus = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION    irpStack  = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION     extension = DeviceObject->DeviceExtension;
	UINT dwDataWritten = 0;

    switch(irpStack->Parameters.DeviceIoControl.IoControlCode)
    {
		case IOCTL_CAPTURE_GET_REGEVENTS:
			ntStatus = HandleIoctlGetRegEvents(DeviceObject, Irp, irpStack, &dwDataWritten);
			break;
		case IOCTL_CAPTURE_START:
			if(extension->bRunning == FALSE)
			{
				CmRegisterCallback(RegistryCallback, extension, &Cookie);
				ntStatus = STATUS_SUCCESS;
				extension->bRunning = TRUE;
			}
			break;
		case IOCTL_CAPTURE_STOP:
			if(extension->bRunning == TRUE)
			{
				CmUnRegisterCallback(Cookie);
				ntStatus = STATUS_SUCCESS;
				extension->bRunning = FALSE;
			}
			break;
        default:
            break;
    }

    Irp->IoStatus.Status = ntStatus;
   
    // Set # of bytes to copy back to user-mode...
    if(ntStatus == STATUS_SUCCESS)
        Irp->IoStatus.Information = dwDataWritten;
    else
        Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return ntStatus;
}

// Removes registry events from the linked list and copies them into the userspace
// buffer that was passed to it.
NTSTATUS HandleIoctlGetRegEvents(IN PDEVICE_OBJECT DeviceObject, PIRP Irp, 
       PIO_STACK_LOCATION pIoStackIrp, UINT *pdwDataWritten)
{
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
    PCHAR pOutputBuffer;
    UINT dwDataWritten = 0;
	// TODO hardcoded for the time being but make the user process define how much space is being
	// passed in the buffer to store events
    UINT dwDataSize = sizeof(REGISTRY_EVENT)*500;
	PDEVICE_EXTENSION     extension = DeviceObject->DeviceExtension;
	*pdwDataWritten = 0;
    pOutputBuffer = Irp->UserBuffer;
	//DbgPrint("Count = %i \r\n", extension->count);
    if(pOutputBuffer)
    {
        __try {
			ProbeForWrite(pOutputBuffer, pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength, __alignof (REGISTRY_EVENT));
			if(pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength >= dwDataSize)
				{
				PLIST_ENTRY head;
				PREGISTRY_EVENT_PACKET packet;
				UINT left = pIoStackIrp->Parameters.DeviceIoControl.OutputBufferLength;
				UINT done = 0;
				
				while(!IsListEmpty(&extension->lRegistry) && (done < left))
				{
					head = ExInterlockedRemoveHeadList(&extension->lRegistry, &extension->lRegistrySpinLock);
					packet = CONTAINING_RECORD(head, REGISTRY_EVENT_PACKET, Link);
					extension->lSize--;
					
					RtlCopyMemory(pOutputBuffer+done, &packet->event, sizeof(REGISTRY_EVENT));
					done += sizeof(REGISTRY_EVENT);

					ExFreePoolWithTag ( packet, REGISTRY_PACKET_POOL_TAG );
				}
				
				*pdwDataWritten = done;
				NtStatus = STATUS_SUCCESS;
			}
			else {
				*pdwDataWritten = dwDataSize;
				NtStatus = STATUS_BUFFER_TOO_SMALL;
			} 
        } __except( EXCEPTION_EXECUTE_HANDLER ) {
			DbgPrint("Exception - %i\n", GetExceptionCode());
			NtStatus = GetExceptionCode();     
        }
    }
    return NtStatus;
}

void UnloadDriver(IN PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING  uszDeviceString;
	NTSTATUS        ntStatus;
	PLIST_ENTRY head;
	PDEVICE_EXTENSION     extension = g_pDeviceObject->DeviceExtension;
	PREGISTRY_EVENT_PACKET packet;

	if(extension->bRunning == TRUE)
	{
		CmUnRegisterCallback(Cookie);
		extension->bRunning = FALSE;
	}	

	while(!IsListEmpty(&extension->lRegistry))
	{
		head = ExInterlockedRemoveHeadList(&extension->lRegistry, &extension->lRegistrySpinLock);
		packet = CONTAINING_RECORD(head, REGISTRY_EVENT_PACKET, Link);
		extension->lSize--;			

		ExFreePoolWithTag ( packet, REGISTRY_PACKET_POOL_TAG );
	}

    IoDeleteDevice(DriverObject->DeviceObject);

    RtlInitUnicodeString(&uszDeviceString, L"\\DosDevices\\CaptureRegistryMonitor");
    IoDeleteSymbolicLink(&uszDeviceString);
}