/*
*	PROJECT: Capture
*	FILE: EventHandlerManager.h
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
#pragma once

/*
* EventHandlerManager
*
* The event handler manager is responsible for managing the callbacks received
* in a TDI_SET_EVENT_HANDLER IRP. Its job is to hook the original callbacks
* and replace them with Capture ones. These hooked event handlers will then call
* the original handlers when required while monitoring the information passed through
* it
*
* NOTE: Because we are hooking it is necessary to destroy our hooked event
* handlers and restore the originals. This is done on DestroyEventHandlerManager
*
*/

#ifndef CONNECTION_POOL_TAG
#define CONNECTION_POOL_TAG			'cCM'
#endif

#include "HashMap.h"

#include <ntddk.h>

#include <tdikrnl.h>

#define TDI_EVENT_HANDLERS_MAX		11

// Forward declare the connection manager
typedef struct _ConnectionManager ConnectionManager;

typedef struct _EventHandler EventHandler;
typedef struct _NodeEventHandler NodeEventHandler;
typedef struct _EventHandlerManager EventHandlerManager;

typedef TDI_REQUEST_KERNEL_SET_EVENT TdiEventHandler;

// Event Handler
// This is passed as a context to our hooked event handlers
// It contains the original event handler so it can be called
// when required. node and connection_manager are saved so
// we can monitor the state
struct _EventHandler
{
	PFILE_OBJECT		node;

	ConnectionManager*	connection_manager;
	
	TdiEventHandler		original_handler;
};

struct _EventHandlerManager
{
	BOOLEAN					initialised;
	HashMap					event_handlers;
	NPAGED_LOOKASIDE_LIST	event_handlers_pool;

	ConnectionManager*		connection_manager;
};

// A node event handler
// Maintains all of the hooked event handlers for a specific node
struct _NodeEventHandler
{
	PFILE_OBJECT	node;
	ULONG			num_handlers;
	EventHandler	handlers[TDI_EVENT_HANDLERS_MAX];
};

// Initialise the event handler manager
BOOLEAN InitialiseEventHandlerManager(EventHandlerManager* event_handler_manager, ConnectionManager* connection_manager);

// Destroy the event handler manager
VOID DestroyEventHandlerManager(EventHandlerManager* event_handler_manager);

// Create a node event handler for the specified node
NodeEventHandler* CreateNodeEventHandler(EventHandlerManager* event_handler_manager, PFILE_OBJECT node);

// Remove an already hooked event handler from a node
// @param	event_handler_manager		The event handler to manage the handlers
// @param	hooked_event_handler		The hooked event handler we set previously from a SetEventHandlerCall
// @param	node						The node for with the hooked event handler is for
VOID RemoveEventHandler(EventHandlerManager* event_handler_manager, PTDI_REQUEST_KERNEL_SET_EVENT hooked_event_handler, PFILE_OBJECT node);

// Set a hooked event handler in place of the original handler
// @param	event_handler_manager		The event handler to manage the handlers
// @param	orginal_event_handler		The original event handler from the SET_EVENT IRP
// @param	node						The node to set the hooked event handler for
VOID SetEventHandler(EventHandlerManager* event_handler_manager, PTDI_REQUEST_KERNEL_SET_EVENT original_event_handler, PFILE_OBJECT node);

// Destroy all existing hooked event handlers and restore the originals
VOID DestroyNodeEventHandlerVisitor(PVOID context, UINT key, PVOID value);