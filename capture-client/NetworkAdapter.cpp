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
		printf("Started network adapter\n");
	}
	DebugPrintTrace(L"NetworkAdapter::start() end\n");
}

string NetworkAdapter::getAdapterName() {
	return adapterName;
}

void
NetworkAdapter::stop()
{
	DebugPrintTrace(L"NetworkAdapter::stop() start\n");
	printf("Stopping network adapter\n");
	if(running)
	{
		DebugPrint(L"NetworkAdapter::stop() stopping adapterThread\n");
		adapterThread->stop();
		DWORD dwWaitResult;
		dwWaitResult = adapterThread->wait(5000);
		switch (dwWaitResult) 
		{
        // All thread objects were signaled
        case WAIT_OBJECT_0: 
            DebugPrint(L"NetworkAdapter::stop() stopped adapterThread\n");
			break;
		case WAIT_TIMEOUT:
			DebugPrint(L"NetworkAdapter::stop() stopping adapterThread timed out. Attempting to terminate.\n");
			adapterThread->terminate();
			DebugPrint(L"NetworkAdapter::stop() terminated adapterThread.\n");
			break;
        // An error occurred
        default: 
            printf("NetworkAdapter stopping adapterThread failed (%d)\n", GetLastError());
		} 
		delete adapterThread;
		if(dumpFile != NULL)
			networkPacketDumper->pfn_pcap_dump_close(dumpFile);
		running = false;
	}
	DebugPrintTrace(L"NetworkAdapter::stop() end\n");
}

void
NetworkAdapter::run()
{
	DebugPrintTrace(L"NetworkAdapter::run() start\n");
	int res;
	struct pcap_pkthdr *header;
	const u_char *pkt_data;
	while(!adapterThread->shouldStop() && (res = networkPacketDumper->pfn_pcap_next_ex( adapter, &header, &pkt_data)) >= 0)
	{     
		if(res > 0)
		{
			if(dumpFile != NULL)
			{
				networkPacketDumper->pfn_pcap_dump((unsigned char *) dumpFile, header, pkt_data);
			}
		}
	}

	DebugPrintTrace(L"NetworkAdapter::run() start\n");
}
