/*
 *	PROJECT: Capture
 *	FILE: CaptureFileMonitor.c
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
#include <fltkernel.h>

typedef unsigned int UINT;

#define FILE_DEVICE_UNKNOWN             0x00000022
#define IOCTL_UNKNOWN_BASE              FILE_DEVICE_UNKNOWN
#define IOCTL_CAPTURE_GET_FILEEVENTS	  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_NEITHER,FILE_READ_DATA | FILE_WRITE_DATA)
#define CAPTURE_FILEMON_PORT_NAME                   L"\\CaptureFileMonitorPort"

#define FILE_PACKET_POOL_TAG 'pFI'

FLT_POSTOP_CALLBACK_STATUS PostFileOperationCallback ( IN OUT PFLT_CALLBACK_DATA Data, 
				IN PCFLT_RELATED_OBJECTS FltObjects, 
				IN PVOID CompletionContext, 
				IN FLT_POST_OPERATION_FLAGS Flags);
				
FLT_PREOP_CALLBACK_STATUS
PreFileOperationCallback (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

NTSTATUS FilterUnload ( IN FLT_FILTER_UNLOAD_FLAGS Flags );

NTSTATUS SetupCallback (IN PCFLT_RELATED_OBJECTS  FltObjects,
				IN FLT_INSTANCE_SETUP_FLAGS  Flags,
				IN DEVICE_TYPE  VolumeDeviceType,
				IN FLT_FILESYSTEM_TYPE  VolumeFilesystemType);

NTSTATUS MessageCallback (
    __in PVOID ConnectionCookie,
    __in_bcount_opt(InputBufferSize) PVOID InputBuffer,
    __in ULONG InputBufferSize,
    __out_bcount_part_opt(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferSize,
    __out PULONG ReturnOutputBufferLength
    );

NTSTATUS ConnectCallback(
    __in PFLT_PORT ClientPort,
    __in PVOID ServerPortCookie,
    __in_bcount(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID *ConnectionCookie
    );

VOID DisconnectCallback(
    __in_opt PVOID ConnectionCookie
    );
// Callback that this driver will be listening for. This is sent to the filter manager
// where it is registered
CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
	/*
    { IRP_MJ_CREATE,
      0,
      NULL,
      PostFileOperationCallback,
	  NULL },

    { IRP_MJ_CLOSE,
      0,
      NULL,
      PostFileOperationCallback,
	  NULL },*/

    { IRP_MJ_READ,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback,
	  NULL  },

    { IRP_MJ_WRITE,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback,
	  NULL },

    { IRP_MJ_OPERATION_END }
};

typedef enum _FILE_NOTIFY_CLASS {
    FilePreRead,
    FilePreWrite
} FILE_NOTIFY_CLASS;

/* File event */
typedef struct  _FILE_EVENT {
	FILE_NOTIFY_CLASS type;
	TIME_FIELDS time;
	WCHAR name[1024];
	HANDLE processID;
} FILE_EVENT, *PFILE_EVENT;

/* Storage for file event to be put into a linked list */
typedef struct  _FILE_EVENT_PACKET {
    LIST_ENTRY     Link;
	FILE_EVENT event;

} FILE_EVENT_PACKET, * PFILE_EVENT_PACKET; 

typedef enum _FILEMONITOR_COMMAND {

    GetFileEvents,
	Start,
	Stop

} FILEMONITOR_COMMAND;

typedef struct _FILEMONITOR_MESSAGE {
    FILEMONITOR_COMMAND Command;
} FILEMONITOR_MESSAGE, *PFILEMONITOR_MESSAGE;


const FLT_CONTEXT_REGISTRATION Contexts[] = {

    { FLT_CONTEXT_END }
};

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof(FLT_REGISTRATION),               //  Size
    FLT_REGISTRATION_VERSION,               //  Version
    0,                                      //  Flags

    Contexts,                               //  Context
    Callbacks,                              //  Operation callbacks

    FilterUnload,                        //  FilterUnload

    SetupCallback,                                   //  InstanceSetup
    NULL,                       //  InstanceQueryTeardown
    NULL,                                   //  InstanceTeardownStart
    NULL,                                   //  InstanceTeardownComplete

    NULL,                                   //  GenerateFileName
    NULL,                                   //  GenerateDestinationFileName
    NULL                                    //  NormalizeNameComponent
};

typedef struct _FILEMON 
{
	PDRIVER_OBJECT DriverObject;
	PFLT_FILTER Filter;
	PFLT_PORT ServerPort;
	PFLT_PORT ClientPort;

	LIST_ENTRY lFile;
	KSPIN_LOCK  lFileSpinLock;
	UINT lSize;
	BOOLEAN bRunning;
} FILEMON, *PFILEMON;

/* Global variables */

FILEMON fileMonitor;



/*	Main entry point into the driver, is called when the driver is loaded */
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject, 
	IN PUNICODE_STRING RegistryPath
	)
{
	NTSTATUS status;
	OBJECT_ATTRIBUTES oa;
	PSECURITY_DESCRIPTOR sd;  
    UNICODE_STRING uniString;

	fileMonitor.lSize = 0;
	fileMonitor.bRunning = FALSE;
	KeInitializeSpinLock(&fileMonitor.lFileSpinLock);
	InitializeListHead(&fileMonitor.lFile);
	// Register driver with the filter manager
	status = FltRegisterFilter(DriverObject, &FilterRegistration, &fileMonitor.Filter);

	if (!NT_SUCCESS(status))
		return status;
	status  = FltBuildDefaultSecurityDescriptor( &sd, FLT_PORT_ALL_ACCESS );

	if (!NT_SUCCESS(status))
		return status;

	RtlInitUnicodeString( &uniString, CAPTURE_FILEMON_PORT_NAME );

	InitializeObjectAttributes( &oa,
								&uniString,
								OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
								NULL,
								sd );
	// Create the communications port to communicate with the user space process (pipe)
	status = FltCreateCommunicationPort( fileMonitor.Filter,
                                             &fileMonitor.ServerPort,
                                             &oa,
                                             NULL,
                                             ConnectCallback,
                                             DisconnectCallback,
                                             MessageCallback,
                                             1 );

	FltFreeSecurityDescriptor( sd );
	// Start the filtering or undo everything is something bad happened
	if(NT_SUCCESS(status))
	{
		status = FltStartFiltering(fileMonitor.Filter);
	} else {
		if (fileMonitor.ServerPort != NULL)
			FltCloseCommunicationPort(fileMonitor.ServerPort);
		if (fileMonitor.Filter != NULL)
			FltUnregisterFilter(fileMonitor.Filter);
	}

	return status;
}

FLT_PREOP_CALLBACK_STATUS
PreFileOperationCallback (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
	NTSTATUS status;
	FILE_EVENT_PACKET EventPacket;
	PFILE_EVENT_PACKET fEventPacket;
	
	PFLT_FILE_NAME_INFORMATION nameInfo =NULL;

	fEventPacket = &EventPacket;
	RtlZeroMemory(&EventPacket, sizeof(FILE_EVENT_PACKET));
	
	if (FltObjects->FileObject != NULL) {
		status = FltGetFileNameInformation( Data,
											FLT_FILE_NAME_NORMALIZED |
											FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
											&nameInfo );
		if(NT_SUCCESS(status))
		{
			PFILE_EVENT_PACKET e;
			LARGE_INTEGER CurrentSystemTime;
			LARGE_INTEGER CurrentLocalTime;
			TIME_FIELDS TimeFields;
			KeQuerySystemTime(&CurrentSystemTime);
			ExSystemTimeToLocalTime(&CurrentSystemTime,&CurrentLocalTime);
			RtlTimeToTimeFields(&CurrentLocalTime,&TimeFields);
			RtlCopyMemory(&fEventPacket->event.time, &TimeFields, sizeof(TIME_FIELDS));

			
			if(Data->Iopb->MajorFunction == IRP_MJ_READ)
			{
				fEventPacket->event.type = FilePreRead;
				//DbgPrint("Got Read\n");
			} else if(Data->Iopb->MajorFunction == IRP_MJ_WRITE) {
				fEventPacket->event.type = FilePreWrite;
				//DbgPrint("Got Write\n");
			}
			// NOTE the path name retrieved is the physical path name and not the logical (c:, d: etc)
			// we leave this up to the user space program to translate so as to make the driver
			// faster ... not by much though :)
			RtlCopyMemory(fEventPacket->event.name, nameInfo->Name.Buffer, nameInfo->Name.Length);
			DbgPrint("File: M:%i, m:%i, F:%x, %i:%i -> %wZ\n", Data->Iopb->MajorFunction, Data->Iopb->MinorFunction, Data->Iopb->IrpFlags, FltGetRequestorProcessId(Data),PsGetCurrentProcessId(), &nameInfo->Name); 
			//DbgPrint("Inserted: %ls\n", fEventPacket->event.name);
			if(fileMonitor.bRunning == TRUE)
			{
				if(fileMonitor.lSize < 1000) {
					e = ExAllocatePoolWithTag(NonPagedPool, sizeof(FILE_EVENT_PACKET), FILE_PACKET_POOL_TAG);
					// Check that this is correct ... I don't think it is
					fEventPacket->event.processID = (HANDLE)FltGetRequestorProcessId(Data);
					RtlCopyMemory(e, fEventPacket, sizeof(FILE_EVENT_PACKET));
					fileMonitor.lSize++;
					
					ExInterlockedInsertTailList(&fileMonitor.lFile,&e->Link,&fileMonitor.lFileSpinLock);
				}
			}

					
		} else {
			// TODO try using a different FltGetFileNameInformation -- FLT_FILE_NAME_OPENED maybe ...

			// Initial testing shows that it never actually finds the file name using this method
			// So it may not need to be included and we just ignore the event if we can't get the
			// pathname

			//NTSTATUS lstatus;
            //PFLT_FILE_NAME_INFORMATION lnameInfo;
			//DbgPrint("Couldn't Get\n");

            //lstatus = FltGetFileNameInformation( Data,
			//	FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
			//	&lnameInfo );
		}
	}
	if(nameInfo != NULL)
	{
		FltReleaseFileNameInformation( nameInfo );
	}
    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS PostFileOperationCallback ( IN OUT PFLT_CALLBACK_DATA Data, 
	IN PCFLT_RELATED_OBJECTS FltObjects, 
	IN PVOID CompletionContext, 
	IN FLT_POST_OPERATION_FLAGS Flags)
{
	// You cannot properly get the full path name of a file operation in the postop
	// This is documented in the WDK documentation regarding this function. READ IT ...
	// in partcular the Comments section regarding the IRP level that this can run in.
    return FLT_POSTOP_FINISHED_PROCESSING;
	
}

NTSTATUS FilterUnload ( IN FLT_FILTER_UNLOAD_FLAGS Flags )
{
	FltCloseCommunicationPort( fileMonitor.ServerPort );

	FltUnregisterFilter( fileMonitor.Filter );

	// TODO Clean up linked list

	return STATUS_SUCCESS;
}

NTSTATUS SetupCallback (IN PCFLT_RELATED_OBJECTS  FltObjects,
	IN FLT_INSTANCE_SETUP_FLAGS  Flags,
	IN DEVICE_TYPE  VolumeDeviceType,
	IN FLT_FILESYSTEM_TYPE  VolumeFilesystemType)
{
	// Always return true so that we attach to all known volumes and all future volumes (USB flashdrives etc)
	if (VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM) {

       return STATUS_FLT_DO_NOT_ATTACH;
    }

    return STATUS_SUCCESS;
}

NTSTATUS MessageCallback (
    __in PVOID ConnectionCookie,
    __in_bcount_opt(InputBufferSize) PVOID InputBuffer,
    __in ULONG InputBufferSize,
    __out_bcount_part_opt(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferSize,
    __out PULONG ReturnOutputBufferLength
    )
{	
	NTSTATUS status;
	FILEMONITOR_COMMAND command;
	PLIST_ENTRY head;
	PFILE_EVENT_PACKET packet;
	ULONG left = OutputBufferSize;
	ULONG done = 0;
	PCHAR pOutputBuffer = OutputBuffer; // Assign a size to PVOID

	if ((InputBuffer != NULL) &&
		(InputBufferSize >= (FIELD_OFFSET(FILEMONITOR_MESSAGE,Command) +
                             sizeof(FILEMONITOR_COMMAND))))
	{
        try  {

            command = ((PFILEMONITOR_MESSAGE) InputBuffer)->Command;

        } except( EXCEPTION_EXECUTE_HANDLER ) {
            return GetExceptionCode();
        }
		
		switch(command) {
			case GetFileEvents:
				while(!IsListEmpty(&fileMonitor.lFile) && (done < left))
				{
					head = ExInterlockedRemoveHeadList(&fileMonitor.lFile, &fileMonitor.lFileSpinLock);
					packet = CONTAINING_RECORD(head, FILE_EVENT_PACKET, Link);
					fileMonitor.lSize--;
					
					RtlCopyMemory(pOutputBuffer+done, &packet->event, sizeof(FILE_EVENT));
					done += sizeof(FILE_EVENT);

					ExFreePoolWithTag ( packet, FILE_PACKET_POOL_TAG );
				}
				
				*ReturnOutputBufferLength = done;
				status = STATUS_SUCCESS;
				break;
			case Start:
				if(fileMonitor.bRunning == FALSE)
				{
					fileMonitor.bRunning = TRUE;
				}
				break;
			case Stop:
				if(fileMonitor.bRunning == TRUE)
				{
					fileMonitor.bRunning = FALSE;
				}
				break;
			default:
				status = STATUS_INVALID_PARAMETER;
				break;

		}
	} else {
		status = STATUS_INVALID_PARAMETER;
	}
	return status;
}

NTSTATUS ConnectCallback(
    __in PFLT_PORT ClientPort,
    __in PVOID ServerPortCookie,
    __in_bcount(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID *ConnectionCookie
    )
{
	ASSERT( fileMonitor.ClientPort == NULL );
    fileMonitor.ClientPort = ClientPort;
    return STATUS_SUCCESS;
}

VOID DisconnectCallback(__in_opt PVOID ConnectionCookie)
{
	PLIST_ENTRY head;
	PFILE_EVENT_PACKET packet;
	
	// Free the linked list containing all the left over file events
	while(!IsListEmpty(&fileMonitor.lFile))
	{
		head = ExInterlockedRemoveHeadList(&fileMonitor.lFile, &fileMonitor.lFileSpinLock);
		packet = CONTAINING_RECORD(head, FILE_EVENT_PACKET, Link);
		fileMonitor.lSize--;					

		ExFreePoolWithTag ( packet, FILE_PACKET_POOL_TAG );
	}

	FltCloseClientPort( fileMonitor.Filter, &fileMonitor.ClientPort );
}
