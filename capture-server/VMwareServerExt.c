/*
 *	PROJECT: Capture
 *	FILE: VMwareServerExt.c
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
#include <jni.h>
#include <stdio.h>
#include "capture_VMwareServerExt.h"
#include "vix.h"

JNIEXPORT jint JNICALL Java_capture_VMwareServerExt_nConnect
(JNIEnv *env, jobject obj, jstring serverAddress, jstring user, jstring pass)
{	
	VixError err;
	VixHandle hostHandle = VIX_INVALID_HANDLE;	
	VixHandle jobHandle = VIX_INVALID_HANDLE;
	
	const char * hostname = (*env)->GetStringUTFChars(env, serverAddress, 0);
	const char * username = (*env)->GetStringUTFChars(env, user, 0);
	const char * password = (*env)->GetStringUTFChars(env, pass, 0);

    jobHandle = VixHost_Connect(VIX_API_VERSION,
                                VIX_SERVICEPROVIDER_VMWARE_SERVER,
                                hostname, // *hostName,
                                902, // hostPort,
                                username, // *userName,
                                password, // *password,
                                0, // options,
                                VIX_INVALID_HANDLE, // propertyListHandle,
                                NULL, // *callbackProc,
                                NULL); // *clientData);
    err = VixJob_Wait(jobHandle, 
                      VIX_PROPERTY_JOB_RESULT_HANDLE, 
                      &hostHandle,
                      VIX_PROPERTY_NONE);
	if (err == VIX_OK) {		
		jclass cls = (*env)->GetObjectClass(env, obj); 		
		jfieldID fid = (*env)->GetFieldID(env, cls, "hServer", "I");		
		if(fid != 0) {			
			(*env)->SetIntField(env, obj, fid, hostHandle);		
		}	
	} else {
	  printf("VIX Error on connect in connect " + err);
	}
	(*env)->ReleaseStringUTFChars(env, user, username);
	(*env)->ReleaseStringUTFChars(env, serverAddress, hostname);
	(*env)->ReleaseStringUTFChars(env, pass, password);
	Vix_ReleaseHandle(jobHandle);
	return err;
	
}

JNIEXPORT jint JNICALL Java_capture_VMwareServerExt_nRevertVM
(JNIEnv *env, jobject obj, jstring path)
{
	VixError err;
	VixHandle jobHandle = VIX_INVALID_HANDLE;
	VixHandle vmHandle = VIX_INVALID_HANDLE;
	VixHandle snapshotHandle = VIX_INVALID_HANDLE; 
	VixHandle hostHandle = 0;

	const char * vmPath = (*env)->GetStringUTFChars(env, path, 0);

	jclass cls = (*env)->GetObjectClass(env, obj); 		
	jfieldID fid = (*env)->GetFieldID(env, cls, "hServer", "I");		
	if(fid != 0) {
		jint jintHostHandle = (*env)->GetIntField(env, obj, fid);
		hostHandle = jintHostHandle;
	}	

	jobHandle = VixVM_Open(hostHandle,
                           vmPath,
                           NULL, // VixEventProc *callbackProc,
                           NULL); // void *clientData);
    err = VixJob_Wait(jobHandle, 
                      VIX_PROPERTY_JOB_RESULT_HANDLE, 
                      &vmHandle,
                      VIX_PROPERTY_NONE);
    if (err == VIX_OK) {		
		Vix_ReleaseHandle(jobHandle);		
		err = VixVM_GetRootSnapshot(vmHandle,
					0,
					&snapshotHandle);
		if (VIX_OK == err) {
			jobHandle = VixVM_RevertToSnapshot(vmHandle,
                 snapshotHandle,
                 0, // options
                 VIX_INVALID_HANDLE, // propertyListHandle
                 NULL, // callbackProc
                 NULL); // clientData
			err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
			if(err != VIX_OK) {
			  printf("VIX Error on revertToSnapshot in revert " + err);
			}
		} else {
		  printf("VIX Error on getRootSnapshot in revert " + err);
		}
    } else {
      printf("VIX Error on open in revert " + err);
	}
	Vix_ReleaseHandle(snapshotHandle);
	Vix_ReleaseHandle(jobHandle);
	Vix_ReleaseHandle(vmHandle);
	(*env)->ReleaseStringUTFChars(env, path, vmPath);
	return err;
}

JNIEXPORT jint JNICALL Java_capture_VMwareServerExt_nRunProgramInGuest
(JNIEnv *env, jobject obj, jstring path, jstring guser, jstring gpass, jstring guestcmd, jstring guestopt)
{	
	VixError err;
	VixHandle jobHandle = VIX_INVALID_HANDLE;
	VixHandle vmHandle = VIX_INVALID_HANDLE;
	VixHandle hostHandle = 0;
	
	const char * vmPath = (*env)->GetStringUTFChars(env, path, 0);
	const char * gusername = (*env)->GetStringUTFChars(env, guser, 0);
	const char * gpassword = (*env)->GetStringUTFChars(env, gpass, 0);
	const char * gcmd = (*env)->GetStringUTFChars(env, guestcmd, 0);
	const char * gcmdoptions = (*env)->GetStringUTFChars(env, guestopt, 0);

	jclass cls = (*env)->GetObjectClass(env, obj); 		
	jfieldID fid = (*env)->GetFieldID(env, cls, "hServer", "I");		
	if(fid != 0) {
		jint jintHostHandle = (*env)->GetIntField(env, obj, fid);
		hostHandle = jintHostHandle;
	}

	jobHandle = VixVM_Open(hostHandle,
                           vmPath,
                           NULL, // VixEventProc *callbackProc,
                           NULL); // void *clientData);
    err = VixJob_Wait(jobHandle, 
                      VIX_PROPERTY_JOB_RESULT_HANDLE, 
                      &vmHandle,
                      VIX_PROPERTY_NONE);
    if (err == VIX_OK) {
		Vix_ReleaseHandle(jobHandle);	
		jobHandle = VixVM_LoginInGuest(vmHandle,
				gusername, // userName
				gpassword, // password 
				0, // options
				NULL, // callbackProc
				NULL); // clientData
	
		err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	
		if (err == VIX_OK) {
			Vix_ReleaseHandle(jobHandle);
		
			// Run the target program.
	
			jobHandle = VixVM_RunProgramInGuest(vmHandle,
						gcmd,
						gcmdoptions,
						VIX_RUNPROGRAM_RETURN_IMMEDIATELY, // options,
						VIX_INVALID_HANDLE, // propertyListHandle,
						NULL, // callbackProc,
						NULL); // clientData
	
			err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
			if(err != VIX_OK) {
			  printf("Error on runProgramInGuest in runProgramInGuest " + err);
			}
		} else {
		  printf("Error on loginInGuest in runProgramInGuest " + err);
		}
	} else {
      printf("VIX Error on open in runProgramInGuest " + err);
	}
	Vix_ReleaseHandle(jobHandle);
	Vix_ReleaseHandle(vmHandle);

	(*env)->ReleaseStringUTFChars(env, guser, gusername);
	(*env)->ReleaseStringUTFChars(env, gpass, gpassword);
	(*env)->ReleaseStringUTFChars(env, path, vmPath);
	(*env)->ReleaseStringUTFChars(env, guestcmd, gcmd);
	(*env)->ReleaseStringUTFChars(env, guestopt, gcmdoptions);

	return err;
}

JNIEXPORT jint JNICALL Java_capture_VMwareServerExt_nWaitForToolsInGuest
(JNIEnv *env, jobject obj, jstring path, jint timeout)
{	
	VixError err;
	VixHandle jobHandle = VIX_INVALID_HANDLE;
	VixHandle vmHandle = VIX_INVALID_HANDLE;
	VixHandle hostHandle = 0;

	int timoutSeconds = timeout;
	const char * vmPath = (*env)->GetStringUTFChars(env, path, 0);

	jclass cls = (*env)->GetObjectClass(env, obj); 		
	jfieldID fid = (*env)->GetFieldID(env, cls, "hServer", "I");		
	if(fid != 0) {
		jint jintHostHandle = (*env)->GetIntField(env, obj, fid);
		hostHandle = jintHostHandle;
	}	

	jobHandle = VixVM_Open(hostHandle,
                           vmPath,
                           NULL, // VixEventProc *callbackProc,
                           NULL); // void *clientData);
    err = VixJob_Wait(jobHandle, 
                      VIX_PROPERTY_JOB_RESULT_HANDLE, 
                      &vmHandle,
                      VIX_PROPERTY_NONE);
   	if (err == VIX_OK) {
		Vix_ReleaseHandle(jobHandle);
		jobHandle = VixVM_WaitForToolsInGuest(vmHandle,
						timoutSeconds, // timeoutInSeconds
						NULL, // callbackProc
						NULL); // clientData
		err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
		if(err != VIX_OK) {
		  printf("VIX Error on waitForToolsInGuest in waitForToolsInGuest " + err);
		}
	} else {
	  printf("VIX Error on open in waitForToolsInGuest " + err);
	}
	Vix_ReleaseHandle(jobHandle);
	Vix_ReleaseHandle(vmHandle);
	(*env)->ReleaseStringUTFChars(env, path, vmPath);

	return err;
}

JNIEXPORT jint JNICALL Java_capture_VMwareServerExt_nDisconnect
(JNIEnv *env, jobject obj)
{
	jclass cls = (*env)->GetObjectClass(env, obj); 		
	jfieldID fid = (*env)->GetFieldID(env, cls, "hServer", "I");	
	if(fid != 0) {
		jint jhostHandle = (*env)->GetIntField(env, obj, fid);
		VixHandle hostHandle = (VixHandle)jhostHandle;
		
		VixHost_Disconnect(hostHandle);		
		if(fid != 0) {			
			(*env)->SetIntField(env, obj, fid, -1);
		}	
	}	
	return 0;
}
