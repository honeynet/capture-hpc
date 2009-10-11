/*
*	PROJECT: Capture
*	FILE: Connection.h
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

#include <ntddk.h>

#include <tdikrnl.h>

#ifndef CONNECTION_POOL_TAG
#define CONNECTION_POOL_TAG			'cCM'
#endif

#define TA_ADDRESS_MAX				(sizeof(TA_ADDRESS) + TDI_ADDRESS_LENGTH_OSI_TSAP)
#define TDI_ADDRESS_INFO_MAX		(sizeof(TDI_ADDRESS_INFO) + TDI_ADDRESS_LENGTH_OSI_TSAP)

typedef unsigned char	BYTE;

typedef struct _Connection Connection;

typedef enum _ConnectionState ConnectionState;

// TCP state
enum _ConnectionState
{
	TCP_STATE_CLOSED,
	TCP_STATE_LISTEN,
	TCP_STATE_SYN_SENT,
	TCP_STATE_SYN_RCVD,
	TCP_STATE_ESTABLISHED,
	TCP_STATE_FIN_WAIT_1,
	TCP_STATE_FIN_WAIT_2,
	TCP_STATE_CLOSING,
	TCP_STATE_TIME_WAIT,
	TCP_STATE_CLOSE_WAIT,
	TCP_STATE_LAST_ACK,
	UDP_STATE_CLOSED,
	UDP_STATE_CONNECT,
	UDP_STATE_ESTABLISHED,
	UDP_STATE_SEND,
	UDP_STATE_RECEIVE,
	UDP_STATE_SEND_DATAGRAM,
	UDP_STATE_RECEIVE_DATAGRAM
};

struct _Connection
{
	PFILE_OBJECT		local_node;									// Transport address object
	PFILE_OBJECT		remote_node;								// Connection context object

	BYTE				local_node_address[TDI_ADDRESS_INFO_MAX];	// Max buffer to store raw IP address info
	BYTE				remote_node_address[TDI_ADDRESS_INFO_MAX];

	ConnectionState			state;										// Current state of the TCP connection
	ULONG				process_id;									// Process id that initiated the connection
};

// Initialised the connection object. Does nothing
VOID InitialiseConnection(Connection* connection);

// Destroy the connection object. Does nothing
VOID DestroyConnection(Connection* connection);

// Sets the connections local node
// @param	connection		Connection object to set
// @param	local_node		The local node transport address object to set
VOID SetLocalNode(Connection* connection, PFILE_OBJECT local_node);

// Sets the connections local node
// @param	connection		Connection object to set
// @param	remote_node		The remote node connection context object to set
VOID SetRemoteNode(Connection* connection, PFILE_OBJECT remote_node);

// Copies the data in a TA_ADDRESS into an address buffer
// @param	address_buffer	The buffer to store the address, from a connection object
// @param	address			The address to copy into the buffer
VOID SetNodeAddress(BYTE* address_buffer, PTA_ADDRESS address);

void VPrintf( char* buffer, int buffer_size, const char* format, ... );

// Prints the address info to the debug channel
// @param	address			The address to print
void PrintAddress( PTA_ADDRESS address );

// Converts a TA_ADDRESS to a string in the form of IP Address:Port
// @param	address			The address to convert to a string
// @param	buffer			The buffer to store the address string
// @param	buffer_size		The size of the buffer
VOID AddressToString(PTA_ADDRESS address, char* buffer, unsigned long buffer_size);

// Sets a connections local node address
// @param	connection		The connection to set the address for
// @param	address			The TA_ADDRESS to store in the connection
VOID SetLocalNodeAddress(Connection* connection, PTA_ADDRESS address);

// Sets a connections remote node address
// @param	connection		The connection to set the address for
// @param	address			The TA_ADDRESS to store in the connection
VOID SetRemoteNodeAddress(Connection* connection, PTA_ADDRESS address);

// Completion routine called when the IRP created in GetLocalAddress is completed.
// Do not call directly
// @param	device_object	A pointer to our device
// @param	irp				The irp that was created in GetLocalAddress
// @param	context			Pointer to a GetAddressContext struct
NTSTATUS GetLocalAddressComplete( PDEVICE_OBJECT device_object, PIRP irp, PVOID context );

// Gets the local address for a connection. Sends an IRP to the next device
// driver in the stack and query the address information for the local node
// This function will block if it is called at a sufficient IRQL. This means
// in some rare cases (netbt execution) the local address will not contain
// valid data until the completion routine is called
// @param	connection		The connection to get the local address for
// @param	device_object	The next driver to pass the query IRP on to
VOID GetLocalAddress(Connection* connection, PDEVICE_OBJECT device_object);
