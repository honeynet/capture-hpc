#include "Precompiled.h"

#include "EventController.h"
#include "NetworkMonitor.h"
#include "ProcessManager.h"
#include "EventList.h"
#include "StringHelper.h"

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
	"TCP_STATE_LAST_ACK",
	"UDP_STATE_CLOSED",
	"UDP_STATE_CONNECT",
	"UDP_STATE_ESTABLISHED",
	"UDP_STATE_SEND",
	"UDP_STATE_RECEIVE",
	"UDP_STATE_SEND_DATAGRAM",
	"UDP_STATE_RECEIVE_DATAGRAM"
};

// TCP state
enum TcpState
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

#define GET_SHARED_EVENT_LIST_IOCTL		CTL_CODE( 0x00000022, 0x810, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA )

NetworkMonitor::NetworkMonitor(void)
: Monitor()
{
	wchar_t driver_path[MAX_PATH];
	wchar_t exclusion_list_path[MAX_PATH];

	monitor_running = false;
	driver_installed = false;

	event_count[0] = CreateSemaphore(NULL, 0, 65355, L"CaptureTcpEventCount");
	event_count[1] = CreateSemaphore(NULL, 0, 65355, L"CaptureUdpEventCount");

	// Load exclusion list
	GetFullPathName(L"ConnectionMonitor.exl", MAX_PATH, exclusion_list_path, NULL);
	Monitor::loadExclusionList(exclusion_list_path);

	// Load network monitor kernel driver
	GetFullPathName(L"CaptureConnectionMonitor.sys", MAX_PATH, driver_path, NULL);
	if(Monitor::installKernelDriver(driver_path, L"CaptureConnectionMonitor", L"Capture Connection Monitor"))
	{	
		driver_handle[0] = CreateFile(
					L"\\\\.\\TCPCaptureConnectionMonitor",
					GENERIC_READ | GENERIC_WRITE, 
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					0,                     // Default security
					OPEN_EXISTING,
					FILE_FLAG_OVERLAPPED,  // Perform asynchronous I/O
					0);                    // No template
		driver_handle[1] = CreateFile(
			L"\\\\.\\UDPCaptureConnectionMonitor",
			GENERIC_READ | GENERIC_WRITE, 
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			0,                     // Default security
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,  // Perform asynchronous I/O
			0);                    // No template
		if(driver_handle[0] == INVALID_HANDLE_VALUE ||
			driver_handle[1] == INVALID_HANDLE_VALUE)
		{
			Monitor::unInstallKernelDriver();
			LOG(ERR, "NetworkMonitor: createFile failed: %08x", GetLastError());
		} 
		else 
		{
			driver_installed = true;
		}
	}
}

// NetworkMonitor Destructor 
NetworkMonitor::~NetworkMonitor(void)
{
	stop();
	if(driver_installed)
	{
		driver_installed = false;
		CloseHandle(driver_handle[0]);
		CloseHandle(driver_handle[1]);	
	}

	CloseHandle(event_count[0]);
	CloseHandle(event_count[1]);
	Monitor::unInstallKernelDriver();
}

void
NetworkMonitor::start()
{
	if(!monitor_running &&
		driver_installed)
	{
		monitor_thread = new Thread(this);
		monitor_thread->start("NetworkMonitor");
	}
}

void
NetworkMonitor::stop()
{
	if(monitor_running &&
		driver_installed)
	{
		monitor_running = false;
		monitor_thread->stop();
		delete monitor_thread;
	}
}

void 
NetworkMonitor::processUdpEvents(KernelEventList* udp_event_list)
{
	if( udp_event_list->head )
	{
		ULONG process_id = 0;
		std::wstring process_path;
		std::wstring udp_type = L"";
		std::wstring destination_address;

		KernelEvent* kernel_event = OFFSET_TO_POINTER(KernelEvent*, udp_event_list, udp_event_list->head);

		ULONG data_offset = kernel_event->head;

		while(data_offset)
		{
			DataContainer* data = OFFSET_TO_POINTER(DataContainer*, udp_event_list, data_offset);
			if( data->key == 0)
			{
				TcpState connection_state = (TcpState)*((int*)data->data);

				if(connection_state == UDP_STATE_SEND ||
					connection_state == UDP_STATE_SEND_DATAGRAM)
				{
					udp_type = L"udp-connection";
				}
				else if( connection_state == UDP_STATE_RECEIVE ||
					connection_state == UDP_STATE_RECEIVE_DATAGRAM)
				{
					udp_type = L"udp-listening";
				}
			}
			else if(data->key == 1)
			{
				// Local address
			}
			else if(data->key == 2)
			{
				// Remote address
				const char* address = (char*)data->data;
				destination_address = StringHelper::multiByteStringToWideString(address, strlen(address)+1);
			}
			else if(data->key == 3)
			{
				process_id = *((ULONG*)data->data);
				process_path = ProcessManager::getInstance()->getProcessPath(process_id);
			}
			data_offset = data->next;

			BufferClearBits(&udp_event_list->buffer, data, (sizeof(DataContainer) + data->size));
		}

		if(udp_type.length() > 0 &&
			!Monitor::isEventAllowed(udp_type, process_path, destination_address))
		{
			signal_onConnectionEvent(udp_type, Time::getCurrentTime(), process_id, process_path, destination_address);
		}

		udp_event_list->head = kernel_event->next;

		BufferClearBits(&udp_event_list->buffer, kernel_event, sizeof(KernelEvent));
	}
}

void 
NetworkMonitor::processTcpEvents(KernelEventList* tcp_event_list)
{
	if( tcp_event_list->head )
	{
		ULONG process_id = 0;
		std::wstring process_path;
		std::wstring connection_type = L"";
		std::wstring destination_address;

		KernelEvent* kernel_event = OFFSET_TO_POINTER(KernelEvent*, tcp_event_list, tcp_event_list->head);

		ULONG data_offset = kernel_event->head;

		while(data_offset)
		{
			DataContainer* data = OFFSET_TO_POINTER(DataContainer*, tcp_event_list, data_offset);
			if( data->key == 0)
			{
				TcpState connection_state = (TcpState)*((int*)data->data);
				if(connection_state == TCP_STATE_ESTABLISHED)
				{
					connection_type = L"tcp-connection";
				}
				else if(connection_state == TCP_STATE_LISTEN)
				{
					connection_type = L"tcp-listening";
				}
			}
			else if(data->key == 1)
			{
				// Local address
			}
			else if(data->key == 2)
			{
				// Remote address
				const char* address = (char*)data->data;
				destination_address = StringHelper::multiByteStringToWideString(address, strlen(address)+1);
			}
			else if(data->key == 3)
			{
				process_id = *((ULONG*)data->data);
				process_path = ProcessManager::getInstance()->getProcessPath(process_id);
			}
			data_offset = data->next;

			BufferClearBits(&tcp_event_list->buffer, data, (sizeof(DataContainer) + data->size));
		}

		if(connection_type.length() > 0 &&
			!Monitor::isEventAllowed(connection_type, process_path, destination_address))
		{
			signal_onConnectionEvent(connection_type, Time::getCurrentTime(), process_id, process_path, destination_address);
		}

		tcp_event_list->head = kernel_event->next;

		BufferClearBits(&tcp_event_list->buffer, kernel_event, sizeof(KernelEvent));
	}
}

void 
NetworkMonitor::run()
{
	DWORD status = 0;
	monitor_running = true;
	DWORD received_bytes = 0;
	KernelEventList* tcp_event_list = NULL;
	KernelEventList* udp_event_list = NULL;

	// Send the semaphore to the kernel driver. Receive a pointer to the event list
	// containing the connection events
	BOOL success = DeviceIoControl(driver_handle[0],
		GET_SHARED_EVENT_LIST_IOCTL, 
		event_count[0], 
		sizeof(HANDLE), 
		&tcp_event_list, 
		sizeof(KernelEventList*),
		&received_bytes, 
		NULL);

	success = DeviceIoControl(driver_handle[1],
		GET_SHARED_EVENT_LIST_IOCTL, 
		event_count[1], 
		sizeof(HANDLE), 
		&udp_event_list, 
		sizeof(KernelEventList*),
		&received_bytes, 
		NULL);

	if(tcp_event_list == NULL || udp_event_list == NULL)
		running = false;

	while(running)
	{
		// Wait for an event to be inserted into the list
		status = WaitForMultipleObjects(2, event_count, FALSE, 1000);

		if(status == WAIT_TIMEOUT)
			continue;

		processTcpEvents(tcp_event_list);
		processUdpEvents(udp_event_list);
	}
	monitor_running = false;
}