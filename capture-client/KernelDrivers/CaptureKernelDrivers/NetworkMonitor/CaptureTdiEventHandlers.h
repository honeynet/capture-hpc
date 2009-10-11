/*
*	PROJECT: Capture
*	FILE: CaptureTdiEventHandlers.h
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

NTSTATUS TdiConnectEventHandler(
							IN PVOID  tdiEventContext,
							IN LONG  remoteAddressLength,
							IN PVOID  remoteAddress,
							IN LONG  userDataLength,
							IN PVOID  userData,
							IN LONG  optionsLength,
							IN PVOID  options,
							OUT CONNECTION_CONTEXT  *connectionContext,
							OUT PIRP  *acceptIrp
							);

NTSTATUS TdiDisconnectEventHandler(
							   IN PVOID  tdiEventContext,
							   IN CONNECTION_CONTEXT  connectionContext,
							   IN LONG  disconnectDataLength,
							   IN PVOID  disconnectData,
							   IN LONG  disconnectInformationLength,
							   IN PVOID  disconnectInformation,
							   IN ULONG  disconnectFlags
							   );

NTSTATUS TdiErrorEventHandler(
				 IN PVOID  TdiEventContext,
				 IN NTSTATUS  Status
				 );

NTSTATUS TdiReceiveEventHandler(
							IN PVOID  tdiEventContext,
							IN CONNECTION_CONTEXT  connectionContext,
							IN ULONG  receiveFlags,
							IN ULONG  bytesIndicated,
							IN ULONG  bytesAvailable,
							OUT ULONG  *bytesTaken,
							IN PVOID  tsdu,
							OUT PIRP  *ioRequestPacket
							);

NTSTATUS TdiReceiveDatagramEventHandler(
    IN PVOID TdiEventContext,
    IN LONG SourceAddressLength,
    IN PVOID SourceAddress,
    IN LONG OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
	);

NTSTATUS TdiReceiveExpeditedEventHandler(
	IN PVOID  TdiEventContext,
	IN CONNECTION_CONTEXT  ConnectionContext,
	IN ULONG  ReceiveFlags,
	IN ULONG  BytesIndicated,
	IN ULONG  BytesAvailable,
	OUT ULONG  *BytesTaken,
	IN PVOID  Tsdu,
	OUT PIRP  *IoRequestPacket);

NTSTATUS TdiSendPossibleEventHandler(
	IN PVOID  TdiEventContext,
	IN PVOID  ConnectionContext,
	IN ULONG  BytesAvailable);

NTSTATUS TdiChainedReceiveEventHandler(
	IN PVOID  TdiEventContext,
	IN CONNECTION_CONTEXT  ConnectionContext,
	IN ULONG  ReceiveFlags,
	IN ULONG  ReceiveLength,
	IN ULONG  StartingOffset,
	IN PMDL  Tsdu,
	IN PVOID  TsduDescriptor);

NTSTATUS TdiChainedReceiveDatagramEventHandler(
	IN PVOID  TdiEventContext,
	IN LONG  SourceAddressLength,
	IN PVOID  SourceAddress,
	IN LONG  OptionsLength,
	IN PVOID  Options,
	IN ULONG  ReceiveDatagramFlags,
	IN ULONG  ReceiveDatagramLength,
	IN ULONG  StartingOffset,
	IN PMDL  Tsdu,
	IN PVOID  TsduDescriptor);

NTSTATUS TdiChainedReceiveExpeditedEventHandler(
	IN PVOID  TdiEventContext,
	IN CONNECTION_CONTEXT  ConnectionContext,
	IN ULONG  ReceiveFlags,
	IN ULONG  ReceiveLength,
	IN ULONG  StartingOffset,
	IN PMDL  Tsdu,
	IN PVOID  TsduDescriptor);

NTSTATUS TdiErrorExEventHandler(
	IN PVOID  TdiEventContext,
	IN NTSTATUS  Status,
	IN PVOID  Buffer);