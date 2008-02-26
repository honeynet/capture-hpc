#include "NetworkAdapter.h"
#include "NetworkPacketDumper.h"

NetworkAdapter::NetworkAdapter(NetworkPacketDumper* npDumper, string aName, pcap_t* adap)
{
	networkPacketDumper = npDumper;
	adapterName = aName;
	adapter = adap;
	running = false;

	InitializeCriticalSection(&adapter_thread_lock);
	
}

NetworkAdapter::~NetworkAdapter(void)
{
	stop();
}

void
NetworkAdapter::start()
{
	if(!running)
	{
		running = true;
		char* szLogFileName = new char[1024];
		string logName = "logs\\";
		logName += adapterName;
		logName += ".pcap";
		GetFullPathNameA(logName.c_str(), 1024, szLogFileName, NULL);
		dumpFile = networkPacketDumper->pfn_pcap_dump_open(adapter, szLogFileName);
		adapterThread = new Thread(this);
		string threadName = "NetworkPacketDumper-";
		threadName += adapterName;
		char* t = (char*)threadName.c_str();
		adapterThread->start(t);
		
		delete [] szLogFileName;
	}
}

void
NetworkAdapter::stop()
{
	if(running)
	{
		// Set to false so the thread will exit
		running = false;

		// Wait for the thread to exit (by setting running to false) and delete
		EnterCriticalSection(&adapter_thread_lock);
		
		// Once we are in the critical section we can be sure the thread has not got
		// any handles to resources that we may deadlock on so now we can stop the thread
		// if it hasn't alread
		adapterThread->stop();
		delete adapterThread;

		if(dumpFile != NULL)
		{
			// Close the dump file
			networkPacketDumper->pfn_pcap_dump_close(dumpFile);
		}	

		LeaveCriticalSection(&adapter_thread_lock);
	}
}

void
NetworkAdapter::run()
{
	int res;
	struct pcap_pkthdr *header;
	const u_char *pkt_data;
	EnterCriticalSection(&adapter_thread_lock);
	while( running )
	{
		if((res = networkPacketDumper->pfn_pcap_next_ex( adapter, &header, &pkt_data)) >= 0)
		{     
			if(res > 0)
			{
				if(dumpFile != NULL)
				{
					networkPacketDumper->pfn_pcap_dump((unsigned char *) dumpFile, header, pkt_data);
				}
			}
		}
	}
	LeaveCriticalSection(&adapter_thread_lock);
}
