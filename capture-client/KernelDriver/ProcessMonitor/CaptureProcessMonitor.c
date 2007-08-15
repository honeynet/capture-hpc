/*
 *	PROJECT: Capture
 *	FILE: CaptureProcessMonitor.c
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

/*	
	Capture Process Monitor - Kernel Driver
	Based on code from James M. Finnegan - http://www.microsoft.com/msj/0799/nerd/nerd0799.aspx

	Driver for monitoring process on Windows XP (should work on all NT systems)

	By Ramon Steenson (rsteenson@gmail.com)
*/
#include <ntifs.h>

#define FILE_DEVICE_UNKNOWN             0x00000022
#define IOCTL_UNKNOWN_BASE              FILE_DEVICE_UNKNOWN
#define IOCTL_CAPTURE_GET_PROCINFO    CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0802, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)

#define IOCTL_CAPTURE_START    CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0805, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_CAPTURE_STOP    CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0806, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_CAPTURE_PROC_LIST    CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0807, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)
#define PROCESS_POOL_TAG 'pPR'
#define PROCESS_HASH_SIZE 1024
#define PROCESS_HASH(ProcessId) \
	((UINT)(((UINT)ProcessId) % PROCESS_HASH_SIZE))

typedef unsigned int UINT;
typedef char * PCHAR;
typedef PVOID           POBJECT;

/* Registry event */
typedef struct  _PROCESS_TUPLE {
	HANDLE processID;
	WCHAR name[1024];
} PROCESS_TUPLE, * PPROCESS_TUPLE;

/* Storage for registry event to be put into a linked list */
typedef struct  _PROCESS_PACKET {
    LIST_ENTRY     Link;
	PROCESS_TUPLE proc;

} PROCESS_PACKET, * PPROCESS_PACKET; 

typedef struct _PROCESS_HASH_ENTRY
{
	LIST_ENTRY lProcess;
	KSPIN_LOCK  lProcessSpinLock;
} PROCESS_HASH_ENTRY, * PPROCESS_HASH_ENTRY;

/* Structure to be passed to the kernel driver using openevent */
typedef struct _CallbackInfo
{
	TIME_FIELDS time;
    HANDLE  hParentId;
	WCHAR parentProcPath[1024];
    HANDLE  hProcessId;
	WCHAR procPath[1024];
    BOOLEAN bCreate;
} CALLBACK_INFO, *PCALLBACK_INFO;

/* Context stuff */
typedef struct _DEVICE_EXTENSION 
{
    PDEVICE_OBJECT DeviceObject;
    HANDLE  hProcessHandle;
    PKEVENT ProcessEvent;
	LIST_ENTRY lProcess;
	KSPIN_LOCK  lProcessSpinLock;
	UINT lSize;
    HANDLE  hPParentId;
    HANDLE  hPProcessId;
    BOOLEAN bPCreate;
	BOOLEAN bRunning;
	//TIME_FIELDS time;
	HANDLE hExpectedProcess;
	CALLBACK_INFO process;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

/* Methods */
NTSTATUS KDispatchCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS KDispatchIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

void UnloadDriver(PDRIVER_OBJECT DriverObject);

VOID ProcessCallback(IN HANDLE  hParentId, IN HANDLE  hProcessId, IN BOOLEAN bCreate);

BOOLEAN InsertProcess(PPROCESS_TUPLE processTuple);
BOOLEAN RemoveProcess(HANDLE processId);
PLIST_ENTRY FindProcess(HANDLE processId);
PPROCESS_TUPLE GetProcess(HANDLE processId);

/* Global device object so our process callback can use the information*/
// Shouldn't really be needed as most callbacks we use can pass in a context value
PDEVICE_OBJECT g_pDeviceObject;
PPROCESS_HASH_ENTRY processHashMap[PROCESS_HASH_SIZE];

/*	Main entry point into the driver, is called when the driver is loaded */
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject, 
	IN PUNICODE_STRING RegistryPath
	)
{
    NTSTATUS        ntStatus;
    UNICODE_STRING  uszDriverString;
    UNICODE_STRING  uszDeviceString;
    UNICODE_STRING  uszProcessEventString;

    PDEVICE_OBJECT    pDeviceObject;
    PDEVICE_EXTENSION extension;
	int i;
    
	// Point uszDriverString at the driver name
    RtlInitUnicodeString(&uszDriverString, L"\\Device\\CaptureProcessMonitor");

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
    if(ntStatus != STATUS_SUCCESS) {
		DbgPrint("P: Returning Here 2 %x\n", ntStatus); 
        return ntStatus;
	}

	
	for(i = 0; i < PROCESS_HASH_SIZE; i++)
	{
		PPROCESS_HASH_ENTRY entry = ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESS_HASH_ENTRY), PROCESS_POOL_TAG);
		KeInitializeSpinLock(&entry->lProcessSpinLock);
		InitializeListHead(&entry->lProcess);
		processHashMap[i] = entry;
	}
    
	// Assign extension variable
    extension = pDeviceObject->DeviceExtension;
	extension->bRunning = FALSE;
    
	// Point uszDeviceString at the device name
    RtlInitUnicodeString(&uszDeviceString, L"\\DosDevices\\CaptureProcessMonitor");
    
	// Create symbolic link to the user-visible name
    ntStatus = IoCreateSymbolicLink(&uszDeviceString, &uszDriverString);

    if(ntStatus != STATUS_SUCCESS)
    {
        // Delete device object if not successful
        IoDeleteDevice(pDeviceObject);
		DbgPrint("P: Returning Here 1 %x\n", ntStatus); 
        return ntStatus;
    }

    // Assign global pointer to the device object for use by the callback functions
    g_pDeviceObject = pDeviceObject;

    // Load structure to point to IRP handlers
    DriverObject->DriverUnload                         = UnloadDriver;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = KDispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = KDispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KDispatchIoctl;

    // Create event for user-mode processes to monitor
    RtlInitUnicodeString(&uszProcessEventString, L"\\BaseNamedObjects\\CaptureProcDrvProcessEvent");
    extension->ProcessEvent = IoCreateNotificationEvent (&uszProcessEventString, &extension->hProcessHandle);

	KeInitializeSpinLock(&extension->lProcessSpinLock);
	InitializeListHead(&extension->lProcess);

    // Clear it out
    KeClearEvent(extension->ProcessEvent);

    // When a process is started or killed, this function will call ProcessCallback
    //ntStatus = PsSetCreateProcessNotifyRoutine(ProcessCallback, FALSE);

	// Could also monitor thread created here ... TODO later
	// Just add a ThreadCallback Method, and mondify the PDEVICE Extension

    // Return success
	DbgPrint("P: Returning %x\n", ntStatus); 
    return ntStatus;
}

NTSTATUS KDispatchCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information=0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

BOOLEAN InsertProcess(PPROCESS_TUPLE processTuple)
{
	UINT hash = PROCESS_HASH(processTuple->processID);

	PPROCESS_HASH_ENTRY entry = processHashMap[hash];

	PPROCESS_PACKET procPacket = ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESS_PACKET), PROCESS_POOL_TAG);

	RtlCopyMemory(&procPacket->proc, processTuple, sizeof(PROCESS_TUPLE));

	RemoveProcess(processTuple->processID);

	ExInterlockedInsertTailList(&entry->lProcess,&procPacket->Link,&entry->lProcessSpinLock);
	
	return TRUE;
}

BOOLEAN RemoveProcess(HANDLE processId)
{
	PLIST_ENTRY entry = FindProcess(processId);
	if(entry != NULL)
	{
		RemoveEntryList(entry);
		ExFreePoolWithTag ( entry, PROCESS_POOL_TAG );
		return TRUE;
	}
	return FALSE;
}

PLIST_ENTRY FindProcess(HANDLE processId)
{
	UINT hash = PROCESS_HASH(processId);

	PPROCESS_HASH_ENTRY entry = processHashMap[hash];

	PLIST_ENTRY pList;
	PPROCESS_PACKET pPacket;

	pList = entry->lProcess.Flink;

    while (pList != &entry->lProcess){

        pPacket = CONTAINING_RECORD( pList, PROCESS_PACKET, Link );

        if (processId == pPacket->proc.processID) {

            return pList;
        }

        pList = pList->Flink;
    }
	return NULL;
}

PPROCESS_TUPLE GetProcess(HANDLE processId)
{
	PLIST_ENTRY entry = FindProcess(processId);
	if(entry != NULL)
	{
		PPROCESS_PACKET packet = CONTAINING_RECORD(entry, PROCESS_PACKET, Link);
		return &packet->proc;
		//DbgPrint("Process tuple delete from hashmap");
		//RemoveEntryList(entry);
		//return TRUE;
	}
	return NULL;
}

/*	
	Process Callback that is called every time a process event occurs. Creates
	a kernel event which can be used to notify userspace processes. 
*/
VOID ProcessCallback(
	IN HANDLE  hParentId, 
	IN HANDLE  hProcessId, 
	IN BOOLEAN bCreate
	)
{
    PDEVICE_EXTENSION extension;
	LARGE_INTEGER CurrentSystemTime;
	LARGE_INTEGER CurrentLocalTime;
	TIME_FIELDS TimeFields;
	KeQuerySystemTime(&CurrentSystemTime);
	ExSystemTimeToLocalTime(&CurrentSystemTime,&CurrentLocalTime);
	RtlTimeToTimeFields(&CurrentLocalTime,&TimeFields);
	

    // Assign extension variable
    extension = g_pDeviceObject->DeviceExtension;
	RtlCopyMemory(&extension->process.time, &TimeFields, sizeof(TIME_FIELDS));

    extension->hPParentId  = hParentId;
    extension->hPProcessId = hProcessId;
    extension->bPCreate    = bCreate;

	if(bCreate) {
		extension->hExpectedProcess = hProcessId;
	} else {
		PPROCESS_TUPLE procTuple;
		PPROCESS_TUPLE parentProcTuple;

		parentProcTuple = GetProcess(hParentId);
		procTuple = GetProcess(hProcessId);
		extension->process.bCreate = bCreate;

		if(procTuple != NULL) {
			DbgPrint("PROC: 0 %i -> %ls", procTuple->processID, procTuple->name);
			RtlCopyMemory(extension->process.procPath, procTuple->name, sizeof(procTuple->name));
			extension->process.hProcessId = hProcessId;

		}
		if(parentProcTuple != NULL) {
			DbgPrint("PARENT: %i -> %ls", parentProcTuple->processID, parentProcTuple->name);
			RtlCopyMemory(extension->process.parentProcPath, parentProcTuple->name, sizeof(parentProcTuple->name));	
			extension->process.hParentId = hParentId;
		}
			
		KeSetEvent(extension->ProcessEvent, 0, FALSE);
		KeClearEvent(extension->ProcessEvent);
	
		//RemoveProcess(hProcessId);
		extension->hExpectedProcess = 0;
	}
}

VOID ProcessImageCallback (
    IN PUNICODE_STRING  FullImageName,
    IN HANDLE  ProcessId, // where image is mapped
    IN PIMAGE_INFO  ImageInfo
    )
{
	PDEVICE_EXTENSION extension;
	extension = g_pDeviceObject->DeviceExtension;
	if((extension->hExpectedProcess == ProcessId) && (ProcessId > 0))
	{
		PROCESS_TUPLE tuple;		
		PPROCESS_TUPLE procTuple = &tuple;
		PPROCESS_TUPLE parentProcTuple;

		RtlZeroMemory(&tuple, sizeof(PROCESS_TUPLE));
		RtlCopyMemory(procTuple->name, FullImageName->Buffer, FullImageName->Length);
		procTuple->processID = ProcessId;
		extension->process.bCreate = extension->bPCreate;
		
		DbgPrint("PROC: 1 %i -> %ls", procTuple->processID, procTuple->name);
		RtlCopyMemory(extension->process.procPath, procTuple->name, sizeof(procTuple->name));
		extension->process.hProcessId = ProcessId;
		InsertProcess(procTuple);
		
		extension->hExpectedProcess = 0;

		parentProcTuple = GetProcess(extension->hPParentId);

		if(parentProcTuple != NULL)
		{
			DbgPrint("PARENT: %i -> %ls", parentProcTuple->processID, parentProcTuple->name);
			RtlCopyMemory(extension->process.parentProcPath, parentProcTuple->name, sizeof(parentProcTuple->name));	
			extension->process.hParentId = parentProcTuple->processID;
		} else {
			DbgPrint("PARENT: Could not find");
		}

		KeSetEvent(extension->ProcessEvent, 0, FALSE);
		KeClearEvent(extension->ProcessEvent);
	}	
}


NTSTATUS KDispatchIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS              ntStatus = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION    irpStack  = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION     extension = DeviceObject->DeviceExtension;
    PCALLBACK_INFO        pCallbackInfo;
	PCHAR pOutputBuffer;
	PPROCESS_TUPLE pInputBuffer;
	UINT dwDataWritten = 0;

    switch(irpStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_CAPTURE_GET_PROCINFO:
            if(irpStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(CALLBACK_INFO))
            {
				pOutputBuffer = Irp->UserBuffer;
				__try {
					ProbeForWrite(pOutputBuffer, 
								irpStack->Parameters.DeviceIoControl.OutputBufferLength,
								__alignof (CALLBACK_INFO));
					RtlCopyMemory(pOutputBuffer, &extension->process, sizeof(CALLBACK_INFO));
					dwDataWritten = sizeof(CALLBACK_INFO);
					ntStatus = STATUS_SUCCESS;
				} __except( EXCEPTION_EXECUTE_HANDLER ) {
					DbgPrint("IOCTL_CAPTURE_GET_PROCINFO Exception - %x\n", GetExceptionCode());
					ntStatus = GetExceptionCode();     
				}        
            }
            break;
		case IOCTL_CAPTURE_START:
			if(extension->bRunning == FALSE)
			{
				ntStatus = PsSetCreateProcessNotifyRoutine(ProcessCallback, FALSE);
				ntStatus = PsSetLoadImageNotifyRoutine(ProcessImageCallback);
				extension->bRunning = TRUE;
			}
			break;
		case IOCTL_CAPTURE_STOP:
			if(extension->bRunning == TRUE)
			{
				ntStatus = PsSetCreateProcessNotifyRoutine(ProcessCallback, TRUE);
				PsRemoveLoadImageNotifyRoutine(ProcessImageCallback);
				extension->bRunning = FALSE;
			}
			break;
		case IOCTL_CAPTURE_PROC_LIST:
		{
			UINT left = irpStack->Parameters.DeviceIoControl.InputBufferLength;
			UINT done = 0;
			pInputBuffer = irpStack->Parameters.DeviceIoControl.Type3InputBuffer;
			__try {	
				ProbeForRead(pInputBuffer,
                    irpStack->Parameters.DeviceIoControl.InputBufferLength, 
                    __alignof(PROCESS_TUPLE));
				if(left >= sizeof(PROCESS_TUPLE))
				{
					PROCESS_TUPLE tuple;		
					PPROCESS_TUPLE procTuple = &tuple;
					RtlZeroMemory(procTuple, sizeof(PROCESS_TUPLE));
					RtlCopyMemory(procTuple, pInputBuffer, sizeof(PROCESS_TUPLE));
					DbgPrint("EXISTING PROC ADDED: %i -> %ls", procTuple->processID, procTuple->name);
					InsertProcess(procTuple);
				}
				ntStatus = done;
			} __except( EXCEPTION_EXECUTE_HANDLER ) {
				DbgPrint("IOCTL_CAPTURE_PROC_LIST Exception - %x\n", GetExceptionCode());
				ntStatus = GetExceptionCode();     
			}
			break;
		}
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

void UnloadDriver(IN PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING  uszDeviceString;
	NTSTATUS        ntStatus;
	PDEVICE_EXTENSION     extension = g_pDeviceObject->DeviceExtension;
	int i;

    // Remove the callback routine
	if(extension->bRunning)
	{
		ntStatus = PsSetCreateProcessNotifyRoutine(ProcessCallback, TRUE);
		PsRemoveLoadImageNotifyRoutine(ProcessImageCallback);
		extension->bRunning = FALSE;
	}

	for(i = 0; i < PROCESS_HASH_SIZE; i++)
	{
		PPROCESS_HASH_ENTRY entry = processHashMap[i];
		PLIST_ENTRY head;
		PPROCESS_PACKET packet;

		while(!IsListEmpty(&entry->lProcess))
		{
			head = ExInterlockedRemoveHeadList(&entry->lProcess, &entry->lProcessSpinLock);
			packet = CONTAINING_RECORD(head, PROCESS_PACKET, Link);
			ExFreePoolWithTag ( packet, PROCESS_POOL_TAG );
		}
		ExFreePoolWithTag ( entry, PROCESS_POOL_TAG );
	}

    IoDeleteDevice(DriverObject->DeviceObject);

    RtlInitUnicodeString(&uszDeviceString, L"\\DosDevices\\CaptureProcessMonitor");
    IoDeleteSymbolicLink(&uszDeviceString);
}