/*
*	PROJECT: Capture
*	FILE: CaptureTdiIrpHandlers.h
*	AUTHORS: Ramon Steenson (rsteenson@gmail.com)
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
#pragma once
#include "ntifs.h"
#include "ntddk.h"

typedef struct _ConnectionManager ConnectionManager;
typedef struct _CompletionRoutine CompletionRoutine;

// TDI IRP handlers
NTSTATUS	TdiDefaultHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiAssociateAddressHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiDisassociateAddressHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiConnectHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiListenHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiAcceptHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiDisconnectHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiSendHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiReceiveHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiSendDatagramHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiReceiveDatagramHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiSetEventHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiQueryInformationHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiSetInformationHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiActionHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );

NTSTATUS	TdiCreateHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiCloseHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );
NTSTATUS	TdiCleanupHandler( ConnectionManager* connection_manager, PIRP irp, PIO_STACK_LOCATION irp_stack, CompletionRoutine* completion_routine );