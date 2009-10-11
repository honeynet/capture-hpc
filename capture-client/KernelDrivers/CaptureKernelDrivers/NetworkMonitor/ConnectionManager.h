/*
*	PROJECT: Capture
*	FILE: ConnectionManager.h
*	AUTHORS:	Ramon Steenson (rsteenson@gmail.com)
*				Van Lam Le (vanlamle@gmail.com)
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

#ifndef CONNECTION_POOL_TAG
#define CONNECTION_POOL_TAG			'cCM'
#endif

#include "Connection.h"
#include "HashMap.h"
#include "EventHandlerManager.h"
#include "EventList.h"

#include <ntddk.h>

typedef struct _ConnectionManager ConnectionManager;
typedef struct _ConnectionNode ConnectionNode;

#define TCP_DEVICE		(0x01)
#define UDP_DEVICE		(0x02)

struct _ConnectionManager
{
	BOOLEAN					intialised;
	UCHAR					device_type;

	PDEVICE_OBJECT			device;
	PDEVICE_OBJECT			next_device;

	HashMap					connections;

	SharedEventList			event_list;

	EventHandlerManager	event_handler_manager;

	NPAGED_LOOKASIDE_LIST	connections_pool;
	NPAGED_LOOKASIDE_LIST	connection_nodes_pool;
};

struct _ConnectionNode
{
	PFILE_OBJECT	node;
	Connection*		connection;
};

// Initialise the connection manager
BOOLEAN InitialiseConnectionManager(ConnectionManager* connection_manager);

// Destroy the connection manager
VOID DestroyConnectionManager(ConnectionManager* connection_manager);

// Finds a connection based on a node which can either be a local or remote node
// @param	connection_manager		The connection manager to search for a connection
// @param	node					The node to use to look for a connection
//
// @returns	A connection object if a node has a valid connection or NULL if none exists
Connection* GetConnection(ConnectionManager* connection_manager, PFILE_OBJECT node);

// Creates a connection.
Connection* CreateConnection(ConnectionManager* connection_manager);

// Release the node from the connection and destorys the ConnectionNode pointing to it
// @param	connection_manager		The connection manager to remove the node from
// @param	connection				The connection to remove the node from
// @param	node					The node to remove form the connection
VOID ReleaseConnectionNode(ConnectionManager* connection_manager, Connection* connection, PFILE_OBJECT node);


// Remove the node and and its connection, called when IRP_MJ_CLEANUP
// @param	connection_manager		The connection manager to remove the node from
// @param	node					The node to remove form the connection
VOID RemoveConnectionNode(ConnectionManager* connection_manager, PFILE_OBJECT node);

// Deletes a connection
VOID RemoveConnection(ConnectionManager* connection_manager, Connection* connection);

// Adds a node to a connection. Can be either a remote or local. If a connection is
// specified the ConnectionNode created will point to the connection otherwise it
// will point to NULL. This is because a ConnectionNode is created before a connection
// is actually established. See IRP_MJ_CREATE handler for more details
// @param	connection_manager		The connection manager to add the connection node to
// @param	connection				The connection the node points to (optional)
// @param	node					The node
VOID AddConnectionNode(ConnectionManager* connection_manager, Connection* connection, PFILE_OBJECT node);

// Sets a connections local node
VOID SetLocalConnectionNode(ConnectionManager* connection_manager, Connection* connection, PFILE_OBJECT local_node);

// Sets a connections remote node
VOID SetRemoteConnectionNode(ConnectionManager* connection_manager, Connection* connection, PFILE_OBJECT remote_node);

// Sets a connections TCP state. This will add an event to the SharedEventList of the connection
// manager and will be picked up the the userspace client.
VOID SetConnectionState( ConnectionManager* connection_manager, Connection* connection, ConnectionState tcp_state);