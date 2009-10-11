/*
 *	PROJECT: Capture
 *	FILE: CaptureConnectionMonitor.h
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

#pragma once
#include "ntifs.h"
#include "ntddk.h"
#include "tdikrnl.h"

#include "ConnectionManager.h"
#include "Connection.h"

typedef unsigned int UINT;

#define CONNECTION_POOL_TAG			'cCM'

typedef struct _CompletionRoutine CompletionRoutine;
typedef struct _CompletionRoutineContext CompletionRoutineContext;

struct _CompletionRoutine
{
	PIO_COMPLETION_ROUTINE callback;
	PVOID context;
	UCHAR control;
};

struct _CompletionRoutineContext
{
	CompletionRoutine completion_routine;
	PIRP original_irp;
};

/// Kernel driver stuff
NTSTATUS DriverEntry( IN PDRIVER_OBJECT driver_object, IN PUNICODE_STRING registry_path );
VOID UnloadDriver( IN PDRIVER_OBJECT driver_object );

/// Handles the IRPs passed to the driver
NTSTATUS HandleIrp( IN PDEVICE_OBJECT device_object, 
				   IN PIRP irp );

/// TDI IRP Completion Handlers
NTSTATUS	TdiCompletionHandler(PDEVICE_OBJECT device_object, PIRP irp, PVOID context);
NTSTATUS	TdiNewIrpCompletionHandler(PDEVICE_OBJECT device_object, PIRP irp, PVOID context);

/// A small wrapper that handles the calling of completion routines we set for IRPs
/// Will call the completion routine stored inside CompletionRoutine only if the status
/// matches what the IRP returned
NTSTATUS	TdiCallCompletionRoutine(CompletionRoutine* completion_routine, PDEVICE_OBJECT device_object, PIRP irp);

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
												CompletionRoutineContext* context);

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
										  CompletionRoutineContext* context);
