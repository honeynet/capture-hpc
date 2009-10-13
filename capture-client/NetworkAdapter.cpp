#include "Precompiled.h"

#include "NetworkAdapter.h"
#include "NetworkPacketDumper.h"

NetworkAdapter::NetworkAdapter(NetworkPacketDumper* npDumper, string aName, pcap_t* adap)
{
	networkPacketDumper = npDumper;
	adapterName = aName;
	adapter = adap;
	running = false;

}

NetworkAdapter::~NetworkAdapter(void)
{
	stop();
}

void
NetworkAdapter::start()
{
	LOG(INFO, "Starting network adapter\n");
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
		LOG(INFO, "Started network adapter\n");
	}
}

string NetworkAdapter::getAdapterName() {
	return adapterName;
}

void
NetworkAdapter::stop()
{
	LOG(INFO, "Stopping network adapter\n");
	if(running)
	{
		LOG(INFO, "NetworkAdapter::stop() stopping adapterThread\n");
		adapterThread->stop();
		DWORD dwWaitResult;
		dwWaitResult = adapterThread->wait(5000);
		switch (dwWaitResult) 
		{
        // All thread objects were signaled
        case WAIT_OBJECT_0: 
            LOG(INFO, "NetworkAdapter::stop() stopped adapterThread\n");
			break;
		case WAIT_TIMEOUT:
			LOG(INFO, "NetworkAdapter::stop() stopping adapterThread timed out. Attempting to terminate.\n");
			adapterThread->terminate();
			LOG(INFO, "NetworkAdapter::stop() terminated adapterThread.\n");
			break;
        // An error occurred
        default: 
            LOG(INFO, "NetworkAdapter stopping adapterThread failed (%d)\n", GetLastError());
		} 
		delete adapterThread;
		if(dumpFile != NULL)
			networkPacketDumper->pfn_pcap_dump_close(dumpFile);
		running = false;
	}
}

void
NetworkAdapter::run()
{
	int res;
	struct pcap_pkthdr *header;
	const u_char *pkt_data;
	while(running && (res = networkPacketDumper->pfn_pcap_next_ex( adapter, &header, &pkt_data)) >= 0)
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
