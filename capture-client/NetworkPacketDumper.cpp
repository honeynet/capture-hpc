#include "Precompiled.h"

#include "NetworkPacketDumper.h"
#include "NetworkAdapter.h"

NetworkPacketDumper::NetworkPacketDumper(void)
{
	monitorRunning = false;
	driverInstalled = false;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_if_t *allDevices;
	pcap_if_t *device;

	hModWinPcap = LoadLibrary(L"wpcap.dll");
	if(hModWinPcap == NULL)
	{
		LOG(INFO, "NetworkPacketDumper: ERROR - wpcap.dll not found. Check that winpcap is installed on this system\n");
	} else {
		pfn_pcap_findalldevs = (pcap_findalldevs_c)GetProcAddress(hModWinPcap, "pcap_findalldevs");
		pfn_pcap_open_live = (pcap_open_live_c)GetProcAddress(hModWinPcap, "pcap_open_live");
		pfn_pcap_close = (pcap_close_c)GetProcAddress(hModWinPcap, "pcap_close");
		pfn_pcap_dump_open = (pcap_dump_open_c)GetProcAddress(hModWinPcap, "pcap_dump_open");	
		pfn_pcap_freealldevs = (pcap_freealldevs_c)GetProcAddress(hModWinPcap, "pcap_freealldevs");
		pfn_pcap_dump_close = (pcap_dump_close_c)GetProcAddress(hModWinPcap, "pcap_dump_close");
		pfn_pcap_next_ex = (pcap_next_ex_c)GetProcAddress(hModWinPcap, "pcap_next_ex");
		pfn_pcap_dump = (pcap_dump_c)GetProcAddress(hModWinPcap, "pcap_dump");
		
		if(pfn_pcap_findalldevs != NULL || pfn_pcap_open_live != NULL || pfn_pcap_close != NULL ||
			pfn_pcap_dump_open != NULL || pfn_pcap_freealldevs != NULL || pfn_pcap_dump_close != NULL ||
			pfn_pcap_next_ex != NULL || pfn_pcap_dump != NULL )
		{
			driverInstalled = true;
		} else {
			LOG(INFO, "NetworkPacketDumper: ERROR - incorrect version of wpcap.dll. Check the correct version of winpcap installed\n");
		}
	}

	if(driverInstalled)
	{
		if(pfn_pcap_findalldevs(&allDevices, errbuf) == -1)
		{
			LOG(ERR, "error in pcap_findalldevs: %s\n", errbuf);
		} else {
			LOG(INFO, "Loading network packet dumper\n");
		}
		
		for(device = allDevices; device; device = device->next)
		{
			if(device->name != NULL)
			{
				pcap_t *fp;			
				if ((fp = pfn_pcap_open_live(device->name, 65536, 0, 1000, errbuf)) == NULL)
				{
					LOG(INFO, "\terror could not open network adapter\n");
				} else {
					/* Only start capturing packets for network adapters that have ip addresses */
					for(pcap_addr_t* a = device->addresses; a; a = a->next) 
					{
						if(a->addr->sa_family == AF_INET)
						{
							if (a->addr)
							{
								char * address = inet_ntoa(((struct sockaddr_in *)a->addr)->sin_addr);
								LOG(INFO, "\tnetwork adapter found: %s\n", address);
								NetworkAdapter* networkAdapter = new NetworkAdapter(this,address, fp);
								adapterList.push_back(networkAdapter);
							}
						}
					}

				}
				
			}
		}
		pfn_pcap_freealldevs(allDevices);
	}
}

NetworkPacketDumper::~NetworkPacketDumper(void)
{
	LOG(INFO, "NetworkdPacketDumper delete\n");
	stop();
	std::list<NetworkAdapter*>::iterator it;
	for(it = adapterList.begin(); it != adapterList.end(); it++)
	{
		//(*it)->getAdapter();
		pfn_pcap_close((*it)->getAdapter());
		delete *it;
	}
	FreeLibrary(hModWinPcap);
}

void
NetworkPacketDumper::start()
{
	if(!isMonitorRunning() && isDriverInstalled())
	{
		std::list<NetworkAdapter*>::iterator it;
		for(it = adapterList.begin(); it != adapterList.end(); it++)
		{
			(*it)->start();
			LOG(INFO, "Started network dumper\n");

		}
		monitorRunning = true;
	}
}

void NetworkPacketDumper::deleteAdapterFiles() {
	std::list<NetworkAdapter*>::iterator it;
	for(it = adapterList.begin(); it != adapterList.end(); it++)
	{
		string adapterName = (*it)->getAdapterName();
		char* szLogFileName = new char[1024];
		string logName = "logs\\";
		logName += adapterName;
		logName += ".pcap";
		GetFullPathNameA(logName.c_str(), 1024, szLogFileName, NULL);
		LOG(INFO, "NetworkdPacketDumper deleteAdapterFiles - deleting %s\n",logName.c_str());
		DeleteFileA(szLogFileName);
		delete [] szLogFileName;
	}
	
}

void
NetworkPacketDumper::stop()
{	
	if(isMonitorRunning() && isDriverInstalled())
	{
		std::list<NetworkAdapter*>::iterator it;
		for(it = adapterList.begin(); it != adapterList.end(); it++)
		{
			LOG(INFO, "NetworkPacketDumper::stop() stopping adapter X\n");
			(*it)->stop();
			LOG(INFO, "Stopped network dumper\n");
			LOG(INFO, "NetworkPacketDumper::stop() stopped adapter X\n");
		}
		monitorRunning = false;
	}	
}
