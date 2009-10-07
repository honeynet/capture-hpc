/*
*	PROJECT: Capture
*	FILE: EventList.h
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

/*
* EventList
*
* Same structure as the EventList.h in KernelDrivers/Common
* Only function implemented in BufferClearBits as thats all we should be able todo.
* NOTE: This is actually a large security hole just waiting to be exploited be
* gentle as it needs a lot more work
*
*/
#pragma once

typedef unsigned char BYTE;

typedef struct _Block				Block;
typedef struct _Buffer				Buffer;
typedef struct _DataContainer		DataContainer;
typedef struct _KernelEvent				KernelEvent;
typedef struct _KernelEventList			KernelEventList;

struct _Block
{
	BYTE data[16];
};


#define BUFFER_SIZE				512 * 1024
#define BLOCK_SIZE				sizeof(Block)
#define BUFFER_BITMAP_SIZE		(BUFFER_SIZE / (sizeof(ULONG)*BLOCK_SIZE*8))


struct _Buffer
{
	// Start RTL_BITMAP
	ULONG SizeOfBitMap;
	PULONG Buffer;  
	// End RTL_BITMAP

	ULONG		bitmap_hint_index;
	ULONG		buffer_bitmap[BUFFER_BITMAP_SIZE];
	BYTE		buffer[BUFFER_SIZE];
};

struct _DataContainer
{
	ULONG			next;
	long			key;
	long			type;
	long			size;
	BYTE			data[];
};

struct _KernelEvent
{
	ULONG			next;
	ULONG			previous;
	long			ready;
	ULONG			head;
	ULONG			tail;
};

struct _KernelEventList
{
	ULONG		head;
	ULONG		tail;
	Buffer		buffer;
};

#define ADDRESS_TO_OFFSET(base, address) ((ULONG)((ULONG_PTR)address - (ULONG_PTR)base))
#define OFFSET_TO_POINTER(object, base, address) ((object)((PUCHAR)address + (ULONG)base))

#define ALIGN_DOWN(length, type) \
	((ULONG)(length) & ~(sizeof(type) - 1))

#define ALIGN_UP(length, type) \
	(ALIGN_DOWN(((ULONG)(length) + sizeof(type) - 1), type))


// Got this somewhere on the net, apologies I can't remeber where from.
// Similar to what RtlBitmapClearBits does
VOID
BufferClearBits (Buffer* buffer, PVOID data, ULONG size)
{
	ULONG shift = 0;
	ULONG count = 0;
	PBYTE position = NULL;
	ULONG bitmap_size = buffer->SizeOfBitMap;

	ULONG bits = ALIGN_UP(size, Block) / BLOCK_SIZE;

	ULONG index = ((ULONG)data - (ULONG)buffer->buffer) / (BLOCK_SIZE);

	if (index >= bitmap_size || bits == 0)
		return;

	if (index + bits > bitmap_size)
		bits = bitmap_size - index;

	position = (PBYTE)((PBYTE)buffer->buffer_bitmap + (index / 8));

	while (bits)
	{
		shift = index & 7;

		// Number of bits to clear in this position
		count = (bits > 8 - shift ) ? 8 - shift : bits;

		*position &= ~(~(0xFF << count) << shift);

		position++;
		bits -= count;
		index += count;
	}
}
