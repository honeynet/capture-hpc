/*
 *	PROJECT: Capture
 *	FILE: revert.c
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
#include <stdio.h>
#include "vix.h"

int main(int argc, char * argv[])
{
	
	VixHandle hostHandle = VIX_INVALID_HANDLE;
	VixHandle vmHandle = VIX_INVALID_HANDLE;
	VixHandle snapshotHandle = VIX_INVALID_HANDLE; 

	VixHandle jobHandle = VIX_INVALID_HANDLE;

	VixError err;
 
	
	/*printf("Hostname: ");
	printf(argv[1]);
	printf("\n");
	printf("Username: ");
	printf(argv[2]);
	printf("\n");
	printf("Password: ");
	printf(argv[3]);
	printf("\n");
	printf("VMPath: ");
	printf(argv[4]);
	printf("\n");
	printf("Guest Username: ");
	printf(argv[5]);
	printf("\n");
	printf("Guest Password: ");
	printf(argv[6]);
	printf("\n");
	printf("Guest Cmd: ");
	printf(argv[7]);
	printf("\n");
	printf("Guest Options: ");
	printf(argv[8]);
	printf("\n");*/
 
	// Connect to specified host.
	
	
	
	jobHandle = VixHost_Connect(VIX_API_VERSION,
                               VIX_SERVICEPROVIDER_VMWARE_SERVER,
                               argv[1], // hostName
                               0, // hostPort
                               argv[2], // userName
                               argv[3], // password,
                               0, // options
                               VIX_INVALID_HANDLE, // propertyListHandle
                               NULL, // callbackProc
                               NULL); // clientData
	err = VixJob_Wait(jobHandle,
                     VIX_PROPERTY_JOB_RESULT_HANDLE,
                     &hostHandle,
                     VIX_PROPERTY_NONE);
					 
	if (VIX_OK != err) {		
	  printf("VIX Error on connect in connect: ");
	  printf(Vix_GetErrorText(err,NULL));
	  printf("\n");
	  goto abort;
	} else {
	  //printf("Connected\n");
	}
	Vix_ReleaseHandle(jobHandle);
	jobHandle = VIX_INVALID_HANDLE;
	
	//get handle
	jobHandle = VixVM_Open(hostHandle,
                       argv[4],
                       NULL, // callbackProc
                       NULL); // clientData
	err = VixJob_Wait(jobHandle,
                  VIX_PROPERTY_JOB_RESULT_HANDLE,
                  &vmHandle,
                  VIX_PROPERTY_NONE);
	if (VIX_OK != err) {
	   printf("VIX Error on obtaining handle: ");
	   printf(Vix_GetErrorText(err,NULL));
	   printf("\n");
	   goto abort;
	} else {
	   //printf("Obtained handle\n");
	}
	Vix_ReleaseHandle(jobHandle);
	jobHandle = VIX_INVALID_HANDLE;

	
	err = VixVM_GetRootSnapshot(vmHandle,
					0,
					&snapshotHandle);
	if(VIX_OK != err) {
		printf("VIX Error on obtaining root snapshot: ");
	   printf(Vix_GetErrorText(err,NULL));
	   printf("\n");
		goto abort;
	} else {
		//printf("Obtained snapshot\n");
	}
	
	jobHandle = VixVM_RevertToSnapshot(vmHandle,
                 snapshotHandle,
                 0, // options
                 VIX_INVALID_HANDLE, // propertyListHandle
                 NULL, // callbackProc
                 NULL); // clientData
	err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	if(VIX_OK != err) {
		printf("VIX Error on reverting to snapshot: ");
	   printf(Vix_GetErrorText(err,NULL));
	   printf("\n");
		goto abort;
	} else {
		//printf("Reverted to snapshot\n");
	}
	Vix_ReleaseHandle(jobHandle);
	jobHandle = VIX_INVALID_HANDLE;
	
	// Wait until guest is completely booted.
	jobHandle = VixVM_WaitForToolsInGuest(vmHandle,
                                      60, // timeoutInSeconds
                                      NULL, // callbackProc
                                      NULL); // clientData
	err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	if (VIX_OK != err) {
		printf("VIX Error on waitingForToolsInGuest: ");
	   printf(Vix_GetErrorText(err,NULL));
	   printf("\n");
		goto abort;
	} else {
		//printf("Waited for tools in guest\n");
	}
	Vix_ReleaseHandle(jobHandle);
	jobHandle = VIX_INVALID_HANDLE;
	
	//login
	jobHandle = VixVM_LoginInGuest(vmHandle,
				argv[5], // userName
				argv[6], // password 
				0, // options
				NULL, // callbackProc
				NULL); // clientData
	
	err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	if (VIX_OK != err) {
		printf("VIX Error on login: ");
	   printf(Vix_GetErrorText(err,NULL));
	   printf("\n");
		goto abort;
	} else {
		//printf("Logged into guest\n");
	}
	Vix_ReleaseHandle(jobHandle);
	jobHandle = VIX_INVALID_HANDLE;
	
	//run capture
	jobHandle = VixVM_RunProgramInGuest(vmHandle,
						argv[7],
						argv[8],
						VIX_RUNPROGRAM_RETURN_IMMEDIATELY, // options,
						VIX_INVALID_HANDLE, // propertyListHandle,
						NULL, // callbackProc,
						NULL); // clientData

	err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	if (VIX_OK != err) {
		printf("VIX Error on run program in guest: ");
	   printf(Vix_GetErrorText(err,NULL));
	   printf("\n");
		goto abort;
	} else {
		//printf("Ran program in guest\n");
	}
	
	Vix_ReleaseHandle(snapshotHandle);
	snapshotHandle = VIX_INVALID_HANDLE;

	Vix_ReleaseHandle(vmHandle);
	vmHandle = VIX_INVALID_HANDLE;

	VixHost_Disconnect(hostHandle);
	//printf("Disconnected\n");
	
	Vix_ReleaseHandle(hostHandle);
	hostHandle = VIX_INVALID_HANDLE;
	
	return 0;
	
abort:
	Vix_ReleaseHandle(jobHandle);
	jobHandle = VIX_INVALID_HANDLE;

	Vix_ReleaseHandle(snapshotHandle);
	snapshotHandle = VIX_INVALID_HANDLE;

	Vix_ReleaseHandle(vmHandle);
	vmHandle = VIX_INVALID_HANDLE;

	VixHost_Disconnect(hostHandle);
	printf("E Disconnected\n");
	
	Vix_ReleaseHandle(hostHandle);
	hostHandle = VIX_INVALID_HANDLE;
	
	return -1;
}