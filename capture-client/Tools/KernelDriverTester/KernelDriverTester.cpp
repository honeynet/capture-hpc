/*
*	PROJECT: Capture
*	FILE: KernelDriverTester.cpp
*	AUTHORS: Ramon Steenson (rsteenson@gmail.com)

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

#include "Log.h"
#include "KernelDriver.h"
#include "File.h"

#include <winioctl.h>

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
	ULONG SizeOfBitMap;                     // Number of bits in bit map
    PULONG Buffer;  
	//RTL_BITMAP	bitmap;
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


const CHAR* TcpStateNames[] =
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
	"TCP_STATE_LAST_ACK"
};

#define GET_SHARED_EVENT_LIST_IOCTL		CTL_CODE( 0x00000022, 0x810, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA )

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


int main(int argc, char* argv[])
{
	KernelDriver driver;
	KernelEventList* event_list = NULL;
	bool acquired = KernelDriver::acquirePrivilege(KernelDriver::load_driver_privilege);

	std::wstring kernel_driver_path = L"C:\\CaptureConnectionMonitor.sys";

	HANDLE semaphore = CreateSemaphore(NULL, 0, 65355, L"KernelTestSemaphore");

	bool installed = driver.install(kernel_driver_path, L"CaptureConnectionMonitor", L"Capture Connection Monitor");
		ULONG received_bytes = 0;
		unsigned char* buffer = NULL;
	
	if( installed )
	{
		driver.open(L"\\\\.\\CaptureConnectionMonitor");
		driver.sendAndReceiveData(GET_SHARED_EVENT_LIST_IOCTL, (unsigned char*)semaphore, sizeof(HANDLE), (unsigned char*)&buffer, (ULONG)sizeof(unsigned char*), received_bytes);
		event_list = (KernelEventList*)buffer;
	}

	static bool run = true;
	while(run)
	{
		DWORD err = WaitForSingleObject(semaphore, 1000);

		if( err == WAIT_TIMEOUT )
		{
		}
		else
		{
			if( event_list->head )
			{
				KernelEvent* kernel_event = OFFSET_TO_POINTER(KernelEvent*, event_list, event_list->head);

				ULONG data_offset = kernel_event->head;
				
				while(data_offset)
				{
					DataContainer* data = OFFSET_TO_POINTER(DataContainer*, event_list, data_offset);
					if( data->key == 0)
					{
						printf("%s\n", TcpStateNames[*((int*)data->data)]);
					}
					else if(data->key == 1)
					{
						printf("Local = %s\n", (char*)data->data);
					}
					else if(data->key == 2)
					{
						printf("Remote = %s\n", (char*)data->data);
					}
					data_offset = data->next;

					BufferClearBits(&event_list->buffer, data, (sizeof(DataContainer) + data->size));
				}

				event_list->head = kernel_event->next;

				BufferClearBits(&event_list->buffer, kernel_event, sizeof(KernelEvent));
			}
		}
	}

	return 0;
}

