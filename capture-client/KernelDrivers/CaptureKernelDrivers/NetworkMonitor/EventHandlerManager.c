/*
*	PROJECT: Capture
*	FILE: EventHandlerManager.c
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

#include "EventHandlerManager.h"

#include "ConnectionManager.h"

#include "CaptureTdiEventHandlers.h"

// Capture SET_EVENT handlers
const PVOID TdiEventHandlers[] =
{
	TdiConnectEventHandler,					// TDI_EVENT_CONNECT
	TdiDisconnectEventHandler,				// TDI_EVENT_DISCONNECT
	TdiErrorEventHandler,					// TDI_EVENT_ERROR
	TdiReceiveEventHandler,					// TDI_EVENT_RECEIVE
	TdiReceiveDatagramEventHandler,			// TDI_EVENT_RECEIVE_DATAGRAM 
	TdiReceiveExpeditedEventHandler,		// TDI_EVENT_RECEIVE_EXPEDITED
	TdiSendPossibleEventHandler,			// TDI_EVENT_SEND_POSSIBLE
	TdiChainedReceiveEventHandler,			// TDI_EVENT_CHAINED_RECEIVE
	TdiChainedReceiveDatagramEventHandler,	// TDI_EVENT_CHAINED_RECEIVE_DATAGRAM
	TdiChainedReceiveExpeditedEventHandler,	// TDI_EVENT_CHAINED_RECEIVE_EXPEDITED
	TdiErrorExEventHandler					// TDI_EVENT_ERROR_EX
};


BOOLEAN InitialiseEventHandlerManager(EventHandlerManager* event_handler_manager, ConnectionManager* connection_manager)
{
	event_handler_manager->connection_manager = connection_manager;

	InitialiseHashMap(&event_handler_manager->event_handlers);

	ExInitializeNPagedLookasideList(&event_handler_manager->event_handlers_pool, NULL, NULL, 0, sizeof(NodeEventHandler), CONNECTION_POOL_TAG, 0);

	event_handler_manager->initialised = TRUE;

	return TRUE;
}

// Destroy all existing hooked event handlers and restore the originals
VOID DestroyEventHandlerManager(EventHandlerManager* event_handler_manager)
{
	IterateHashMap(&event_handler_manager->event_handlers, DestroyNodeEventHandlerVisitor, (PVOID)event_handler_manager);

	ExDeleteNPagedLookasideList(&event_handler_manager->event_handlers_pool);

	DestroyHashMap(&event_handler_manager->event_handlers);
}

VOID DestroyNodeEventHandlerVisitor(PVOID context, UINT key, PVOID value)
{
	NTSTATUS status;
	PIRP irp = NULL;
	unsigned int i = 0;
	KEVENT irp_complete_event;
	IO_STATUS_BLOCK io_status_block;
	NodeEventHandler* node_event_handler = NULL;
	EventHandlerManager* event_handler_manager = NULL;

	node_event_handler = (NodeEventHandler*)value;
	event_handler_manager = (EventHandlerManager*)context;

	if(node_event_handler == NULL ||
		event_handler_manager == NULL )
		return;

	KeInitializeEvent(&irp_complete_event, NotificationEvent, FALSE);

	// For each hooked event handler send an IRP to set the event handler
	// back to what it originally was
	for(i = 0; i < TDI_EVENT_HANDLERS_MAX; i++)
	{
		PFILE_OBJECT node = node_event_handler->handlers[i].node;
		TdiEventHandler* original_handler = &node_event_handler->handlers[i].original_handler;

		if(original_handler->EventHandler == NULL)
			continue;

		irp = TdiBuildInternalDeviceControlIrp(TDI_SET_EVENT_HANDLER, event_handler_manager->connection_manager->device, node, 
			&irp_complete_event, &io_status_block);

		if(irp == NULL)
			continue;

		TdiBuildSetEventHandler(irp, event_handler_manager->connection_manager->device, node, NULL, NULL, 
			original_handler->EventType, original_handler->EventHandler, original_handler->EventContext);

		status = IoCallDriver(event_handler_manager->connection_manager->next_device, irp);

		if(status == STATUS_PENDING)                                
		{
			// Wait for the IRP to finish
			KeWaitForSingleObject((PVOID)&irp_complete_event, Executive, KernelMode, TRUE, NULL); 
			status = io_status_block.Status; 
		}

		if(status == STATUS_SUCCESS)
		{
			DbgPrint("EventHandlerManager: Successfully set original event handler n=%08x t=%08x\n", node, original_handler->EventType);
		}
		else
		{
			DbgPrint("EventHandlerManager: Failed to set original event handler - code=%08x n=%08x t=%08x\n", status, node, original_handler->EventType);
		}

		KeResetEvent(&irp_complete_event);
	}
}

// Create a node event handler for the specified node
NodeEventHandler* CreateNodeEventHandler(EventHandlerManager* event_handler_manager, PFILE_OBJECT node)
{
	NodeEventHandler* node_event_handler = NULL;

	node_event_handler = (NodeEventHandler*)ExAllocateFromNPagedLookasideList(&event_handler_manager->event_handlers_pool);

	if(node_event_handler == NULL)
		return NULL;

	node_event_handler->node = node;
	node_event_handler->num_handlers = 0;

	RtlZeroMemory(node_event_handler->handlers, sizeof(EventHandler) * TDI_EVENT_HANDLERS_MAX);

	InsertHashMap(&event_handler_manager->event_handlers, (UINT)node, node_event_handler);

	return node_event_handler;
}

// Remove an already hooked event handler from a node
// @param	event_handler_manager		The event handler to manage the handlers
// @param	hooked_event_handler		The hooked event handler we set previously from a SetEventHandlerCall
// @param	node						The node for with the hooked event handler is for
VOID RemoveEventHandler(EventHandlerManager* event_handler_manager, PTDI_REQUEST_KERNEL_SET_EVENT hooked_event_handler, PFILE_OBJECT node)
{
	EventHandler* event_handler = NULL;
	NodeEventHandler* node_event_handler = NULL;

	if(hooked_event_handler->EventType >= TDI_EVENT_HANDLERS_MAX)
		return;

	node_event_handler = (NodeEventHandler*)FindHashMapValue(&event_handler_manager->event_handlers, (UINT)node);

	if(node_event_handler == NULL)
		return;

	event_handler = &node_event_handler->handlers[hooked_event_handler->EventType];

	// Delete the event handler
	node_event_handler->num_handlers--;
	RtlZeroMemory(event_handler, sizeof(EventHandler));

	// If the node event handler is not managing any handles we might as well delete it
	if(node_event_handler->num_handlers == 0)
	{
		ExFreeToNPagedLookasideList(&event_handler_manager->event_handlers_pool, node_event_handler);

		RemoveHashMap(&event_handler_manager->event_handlers, (UINT)node);
	}
}

// Set a hooked event handler in place of the original handler
// @param	event_handler_manager		The event handler to manage the handlers
// @param	orginal_event_handler		The original event handler from the SET_EVENT IRP
// @param	node						The node to set the hooked event handler for
VOID SetEventHandler(EventHandlerManager* event_handler_manager, PTDI_REQUEST_KERNEL_SET_EVENT original_event_handler, PFILE_OBJECT node)
{
	EventHandler* event_handler = NULL;
	NodeEventHandler* node_event_handler = NULL;

	if(original_event_handler->EventType >= TDI_EVENT_HANDLERS_MAX)
		return;

	// Early exit if no event handler is needed
	if(TdiEventHandlers[original_event_handler->EventType] == NULL)
		return;

	node_event_handler = (NodeEventHandler*)FindHashMapValue(&event_handler_manager->event_handlers, (UINT)node);

	// Create a node event handler if one was not found
	if(node_event_handler == NULL)
	{
		node_event_handler = CreateNodeEventHandler(event_handler_manager, node);
		if(node_event_handler == NULL)
			return;
	}

	event_handler = &node_event_handler->handlers[original_event_handler->EventType];

	// Keep track of the original callback so we can call it in our callback routine
	event_handler->original_handler.EventType = original_event_handler->EventType;
	event_handler->original_handler.EventHandler = original_event_handler->EventHandler;
	event_handler->original_handler.EventContext = original_event_handler->EventContext;

	event_handler->node = node;
	event_handler->connection_manager = event_handler_manager->connection_manager;

	// Set our callback routine and put the our event handler as the context
	original_event_handler->EventContext = event_handler;
	original_event_handler->EventHandler = TdiEventHandlers[original_event_handler->EventType];
	return;
}