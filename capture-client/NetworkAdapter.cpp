#include "NetworkAdapter.h"
#include "NetworkPacketDumper.h"

NetworkAdapter::NetworkAdapter(NetworkPacketDumper* npDumper, string aName, pcap_t* adap)
{
	DebugPrintTrace(L"NetworkAdapter::NetworkAdapter(NetworkPacketDumper* npDumper, string aName, pcap_t* adap) start\n");
	networkPacketDumper = npDumper;
	adapterName = aName;
	adapter = adap;
	running = false;

	DebugPrintTrace(L"NetworkAdapter::NetworkAdapter(NetworkPacketDumper* npDumper, string aName, pcap_t* adap) end\n");
}

NetworkAdapter::~NetworkAdapter(void)
{
	DebugPrintTrace(L"NetworkAdapter::~NetworkAdapter(void) start\n");
	stop();
	DebugPrintTrace(L"NetworkAdapter::~NetworkAdapter(void) end\n");
}

void
NetworkAdapter::start()
{
	printf("Starting network adapter\n");
	DebugPrintTrace(L"NetworkAdapter::start() start\n");
	if(!running)
	{
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
		running = true;
		delete [] szLogFileName;
	}
	DebugPrintTrace(L"NetworkAdapter::start() end\n");
}

void
NetworkAdapter::stop()
{
	printf("Stopping network adapter\n");
	DebugPrintTrace(L"NetworkAdapter::stop() start\n");
	if(running)
	{
		adapterThread->stop();
		delete adapterThread;
		if(dumpFile != NULL)
			networkPacketDumper->pfn_pcap_dump_close(dumpFile);
		//networkPacketDumper->pfn_pcap_close(adapter);
		running = false;
	}
	DebugPrintTrace(L"NetworkAdapter::stop() end\n");
}

void
NetworkAdapter::run()
{
	DebugPrintTrace(L"NetworkAdapter::run() start\n");
	try {
		int res;
		struct pcap_pkthdr *header;
		const u_char *pkt_data;
		while((res = networkPacketDumper->pfn_pcap_next_ex( adapter, &header, &pkt_data)) >= 0)
		{     
			if(res > 0)
			{
				if(dumpFile != NULL)
				{
					networkPacketDumper->pfn_pcap_dump((unsigned char *) dumpFile, header, pkt_data);
				}
			}
		}
	} catch (...) {
		printf("NetworkAdapter::run exception\n");	
		throw;
	}
	DebugPrintTrace(L"NetworkAdapter::run() start\n");
}
