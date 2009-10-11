/*
*	PROJECT: Capture
*	FILE: ConnectionManager.c
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

#include "ConnectionManager.h"

#include "Connection.h"
#include "EventHandlerManager.h"

const CHAR* ConnectionStateNames[] =
{
	"TCP_STATE_CLOSED",
	"TCP_STATE_LISTEN",
	"TCP_STATE_SYN_SENT",
	"TCP_STATE_SYN_RCVD",
	"TCP_STATE_ESTABLISHED",
	"TCP_STATE_FIN_WAIT_1",
	"TCP_STATE_FIN_WAIT_2",
	"TCP_STATE_CLOSING",
	"TCP_STATE_TIME_WAIT",
	"TCP_STATE_CLOSE_WAIT",
	"TCP_STATE_LAST_ACK",
	"UDP_STATE_CLOSE",
	"UDP_STATE_CONNECT",
	"UDP_STATE_ESTABLISHED",
	"UDP_STATE_SEND",
	"UDP_STATE_RECEIVE",
	"UDP_STATE_SEND_DATAGRAM",
	"UDP_STATE_RECEIVE_DATAGRAM"
};

BOOLEAN InitialiseConnectionManager(ConnectionManager* connection_manager)
{
	InitialiseSharedEventList(&connection_manager->event_list);

	InitialiseEventHandlerManager(&connection_manager->event_handler_manager, connection_manager);

	InitialiseHashMap(&connection_manager->connections);

	ExInitializeNPagedLookasideList(&connection_manager->connections_pool, NULL, NULL, 0, sizeof(Connection), CONNECTION_POOL_TAG, 0);
	ExInitializeNPagedLookasideList(&connection_manager->connection_nodes_pool, NULL, NULL, 0, sizeof(ConnectionNode), CONNECTION_POOL_TAG, 0);

	connection_manager->intialised = TRUE;

	return TRUE;
}

VOID DestroyConnectionManager(ConnectionManager* connection_manager)
{
	connection_manager->intialised = FALSE;

	ExDeleteNPagedLookasideList( &connection_manager->connections_pool );
	ExDeleteNPagedLookasideList( &connection_manager->connection_nodes_pool );

	DestroyHashMap(&connection_manager->connections);

	DestroyEventHandlerManager(&connection_manager->event_handler_manager);

	DestroySharedEventList(&connection_manager->event_list);
}

Connection* GetConnection(ConnectionManager* connection_manager, PFILE_OBJECT node)
{
	Connection* connection = NULL;
	ConnectionNode* connection_node = NULL;

	// Find the connection based on the node pointer
	connection_node = (ConnectionNode*)FindHashMapValue( &connection_manager->connections, (UINT)node);

	if( connection_node )
	{
		return connection_node->connection;
	}
	else
	{
		DbgPrint("Can't find connection for node %08x\n",node);
		return NULL;
		// Do we want to create a connection here?
	}


	return NULL;
}

Connection* CreateConnection(ConnectionManager* connection_manager)
{
	Connection* connection = NULL;

	connection = (Connection*)ExAllocateFromNPagedLookasideList( &connection_manager->connections_pool );

	if(connection == NULL)
		return NULL;

	if (connection_manager->device_type==TCP_DEVICE)
		connection->state = TCP_STATE_CLOSED;
	else if (connection_manager->device_type==UDP_DEVICE)
		connection->state= UDP_STATE_CLOSED;

	return connection;
}


VOID ReleaseConnectionNode(ConnectionManager* connection_manager, Connection* connection, PFILE_OBJECT node)
{
	ConnectionNode* connection_node = NULL;

	if(node == NULL)
		return;

	if(connection->local_node == node)
		connection->local_node = NULL;
	if(connection->remote_node == node)
		connection->remote_node = NULL;

	connection_node = (ConnectionNode*)FindHashMapValue( &connection_manager->connections, (UINT)node);

	connection_node->connection = NULL;

	//if(connection_node)
	//{
	// ExFreeToNPagedLookasideList(&connection_manager->connection_nodes_pool, connection_node);
	//}

	//RemoveHashMap(&connection_manager->connections, (UINT)node);
}
VOID RemoveConnectionNode(ConnectionManager* connection_manager, PFILE_OBJECT node)
{
	ConnectionNode* connection_node = NULL;
	connection_node = (ConnectionNode*)FindHashMapValue( &connection_manager->connections, (UINT)node);
	if (connection_node==NULL)
	{
		DbgPrint("Remove connection node error: can't find connection node for object %08x\n",node);
		return;
	}
	else 
	{
		if (connection_node->connection!=NULL)
		{
			DbgPrint("Remove connection node: %08x\n",node);
			RemoveConnection(connection_manager,connection_node->connection);
		}
		RemoveHashMap(&connection_manager->connections, (UINT)node);
		ExFreeToNPagedLookasideList(&connection_manager->connection_nodes_pool, connection_node);
	}
}
VOID RemoveConnection(ConnectionManager* connection_manager, Connection* connection)
{
	if(connection == NULL)
		return;
	if ((connection->local_node!=NULL) && (connection->remote_node!=NULL))
	{
		DbgPrint("Remove connection:%08x\t\tlocal node:%08x\t\tremote node:%08x\n",connection,connection->local_node,connection->remote_node);
	}
	ExFreeToNPagedLookasideList(&connection_manager->connections_pool, connection);
}

VOID AddConnectionNode(ConnectionManager* connection_manager, Connection* connection, PFILE_OBJECT node)
{
	ConnectionNode* connection_node = NULL;

	// Check we haven't already added a node to this connection
	if(FindHashMap(&connection_manager->connections, (UINT)node))
	{
		connection_node = (ConnectionNode*)FindHashMapValue(&connection_manager->connections, (UINT)node);
		//ASSERT(connection == connection_node->connection);	// Sanity check the connections are the same, they should be
		return;
	}

	connection_node = (ConnectionNode*)ExAllocateFromNPagedLookasideList(&connection_manager->connection_nodes_pool);

	if(connection_node == NULL)
		return;

	connection_node->node = node;
	connection_node->connection = connection;

	InsertHashMap(&connection_manager->connections, (UINT)node, connection_node);
}


VOID SetLocalConnectionNode(ConnectionManager* connection_manager, Connection* connection, PFILE_OBJECT local_node)
{
	SetLocalNode(connection, local_node);
	AddConnectionNode(connection_manager, connection, local_node);
}

VOID SetRemoteConnectionNode(ConnectionManager* connection_manager, Connection* connection, PFILE_OBJECT remote_node)
{
	SetRemoteNode(connection, remote_node);
	AddConnectionNode(connection_manager, connection, remote_node);
}

VOID SetConnectionState( ConnectionManager* connection_manager, Connection* connection, ConnectionState tcp_state)
{
	ConnectionState old_state = 0;

	if(connection == NULL)
		return;

	old_state = connection->state;
	connection->state = tcp_state;

	// Create a state changed event and queue
	{
		char buffer[128];
		Event* connection_event = NULL;

		DbgPrint("\t\t\t\t\t\t\t\t\t\t");

		connection_event = CreateEvent( &connection_manager->event_list );

		AddEventData(& connection_manager->event_list, connection_event, 0, 0, sizeof(ConnectionState), &tcp_state );

		AddressToString( (PTA_ADDRESS)connection->local_node_address, buffer, 128 );
		AddEventData( &connection_manager->event_list, connection_event, 1, 1, strlen(buffer) + 1, buffer );

		DbgPrint("%s->", buffer);

		AddressToString( (PTA_ADDRESS)connection->remote_node_address, buffer, 128 );
		AddEventData( &connection_manager->event_list, connection_event, 2, 2, strlen(buffer) + 1, buffer );

		AddEventData( &connection_manager->event_list, connection_event, 3, 3, sizeof(ULONG), &connection->process_id);

		DbgPrint("%s = %s => %s\n", buffer,  ConnectionStateNames[old_state], ConnectionStateNames[tcp_state]);

		InsertEvent( &connection_manager->event_list, connection_event );
	}
}