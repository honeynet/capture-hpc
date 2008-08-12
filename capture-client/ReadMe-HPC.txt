1. Capture Client README
------------------------
Capture allows to find malicious servers on a network. Capture is split into two functional areas: a Capture Server and Capture Client. The Capture clients accept the commands of the server to start and stop themselves and to interact with a server. As a Capture client interacts with a server, it monitors its state for changes to processes that are running, the registry, and the file system. Since some events occur during normal operation (e.g. windows update runs frequently), exclusion lists allow to ignore certain type of events. If changes are detected that are not part of the exclusion list, the client makes a malicious classification of the web server and sends this information to the Capture server. Since the state of the Capture client has been changed, the Capture client resets its state to a clean state before it retrieves new instructions from the Capture server. In case no state changes are detected, the Capture client retrieves new instructions from the Capture server without resetting its state.

Capture is written and distributed under the GNU General Public License.

2. Prerequisites
----------------
The Capture client component runs inside of a virtual machine (guest os). This guide will go through the stages of installing VMware server and configuring the virtual machine through to installing the client on the guest os. 

2.1 Installing VMware Server
----------------------------
Install VMware server using any means available: 

* On Linux, use either you package management software available on your distribution or download it from here http://www.vmware.com/download/server/ 
* On Windows, download the setup software from http://www.vmware.com/download/server/ 

Linux requires some additional adjustments. The ports must be opened, and the VMware server must be allowed to authenticate with the users on the host machine. On Linux, VMware server uses xinetd to authenticate the incoming connections with users on the host machine. This requires that xinetd be set up to accept remote authentication. Do the following: 
1. Open up the file /etc/xinetd.conf (Maybe be different depending on the distribution) 
2. Look for the line labelled only_from and add the IP addresses you are expecting remote authentication from 
@@only_from = 192.168.0.20;192.168.0.2 
On Windows, VMware server should be able to accept incoming connections already. However you may need to open up the port which it listens on (default is 902) in the Windows firewall. 

2.2 Creating a Virtual Machine
------------------------------
Capture uses Windows XP SP2, thus the virtual machine that is to be created is to be Windows XP SP2. 

* Create a virtual machine 
* Install Windows XP SP2 onto the virtual machine 
* Install the VMware tools 
* Install the Microsoft Visual C++ 2008 Redistributable Libraries (SP0)
* Optionally install the winpcap 4.0.2 if network dump functionality is used

3. Installing the Capture Client Component on the Virtual Machine
-----------------------------------------------------------------
Download and execute the Capture client setup. You might be prompted for the location of additional files. These are located in the i386 directory on your Windows XP installation CD.

4. Configure Capture Client Component
-------------------------------------

4.1. Exclusion Lists
--------------------
The capture client currently is able to detect changes to the processes running on the system, modifications to the registry and modifications to the file system. Since the system will generate events during normal operation that are not malicious, exclusion lists are provided to exclude these events from causing a malicious classification of the URI. An empty exclusion list would mean that no events are allowed at all. If an event should be excluded, it can be specified one per line. Via regular expressions, one can group multiple events into one line item. The '+' sign at the beginning of each line specifies that an event should be excluded whereas a '-' sign specifies that an event should be explicitly included. This allows one to, for example, specify that a process is allowed to write into a specific directory, but not .exe files.

There are three exclusion lists that allow to exclude these three event types respectively: ProcessMonitor.exl, FileMonitor.exl, and RegistryMonitor.exl. Default exclusion list are provided with Capture that exclude events that normally occur when browsing benign web pages with IESP2. Because patch levels and third party plug-ins could cause different events to occur on a system, the exclusion lists might have to be adjusted for your environment.

The ProcessMonitor.exl allows to specify processes that should not be triggering a malicious classification. One process should be listed per line. By default, for instance, the client process IEXPLORE is contained in the ProcessMonitor.exl. Without this line item, any URI would be classified as malicious as the IEXPLORE process is created as part of retrieving a URI. 

The FileMonitor.exl allows to specify files that should not be triggering a malicious classification. There is the option to specify whether read, write, create, open, and delete events on specific files should be excluded as well as specifying the process that causes these events. So, for example, one can exclude write and read file events in the temp Internet Cache folder that are caused by Internet Explorer from triggering a malicious classification. However, access by another process on these files would. 

The RegistryMonitor.exl allows to specify registry events that should not be triggering a malicious classification. There is the option to specify whether OpenKey, CreateKey, CloseKey, EnumerateKey, QueryValueKey, QueryKey, SetValueKey, and DeleteValueKey events on specific registry keys or values should be excluded as well as specifying the process that causes these events. 

4.2. Client Configuration
-------------------------
In addition to the exclusion list, one can configure the client identifiers of the client honeypot in the applications.conf file. The client identifiers are pointers to client applications that can be used to interact with a server, such as Internet Explorer, Firefox, Opera, but also other applications that can retrieve content from a network, such as Microsoft Word, Adobe Acrobat Reader. Simply add the client identfier and fully qualifying path name to the client.conf file and specify the client identifier in the capture-server config file.  

Since some applications (like Adobe Acrobat) are unable to retrieve content files directly, Capture might retrieve the files, save them to a temporary folder, and then open the file with the application. To enable such behavior, edit the applications.conf file and specify "yes" at the end of the line.

When using the client identifiers from the applications.conf files, the group size in the Capture Server configuration needs to be set to 1. An exception is firefox, which supports visiting multiple urls at the same time.

The applications.conf file settings can be overwritten by specific plugins. We provide three such a plugins. Some of these plugins are a necessity to make capture operate with the corresponding client; others are pure performance enhancers.

iexplore: This client plugin uses the Internet Explorer component. Its has the ability to relay more information between the capture client and the client application itself. As such, with this plug-in, it is possible to obtain information on when the page has been successfully loaded. This enables us to dynamically adjust the visitation delay to be page-load-time + a fixed visitation delay (this stands in contrast to using client applications directly where the fixed visitation delay has to accommodate the page-load-time). In addition, the plug-in allows to relay HTTP response code back to the capture-client and server. This will allow one to assess which pages loaded successful, which failed and then which of the successful pages solicited malicious behavior.
In addition, the iexplore plug-in allows one to visit multiple URLs at the same time. A divide-and-conquer mechanism is applied in which urls are visited at the same time and when a unauthorized state change is detected, the urls are split into two and revisited until the responsible url is identified. Note, that there is a danger of evasion when directly connecting to the internet, since malicious urls might only trigger on initial contact, but not on subsequent ones. This risk can be reduced by using a proxy that caches all content.

iexplorebulk: This is a client plugin that opens Internet Explorer in its own process. It has the same advantages as iexplore in collecting additional information about interacting with a URL. It is also able to visit multiple URLs are the same time. However, because each Internet Explorer instance is started in its own process, state changes can be mapped to the responsible URL without re-visitation. On the flipside, iexplorebulk uses more resources, because individual instances are created. Also, the malware and network collection does not map to specific URLs, but rather to a group of URLs. Manual filtering of the data would be required to map malware and network traffic to a specific URL. 

safari: This is a simple plugin for Apple's safari browser. Because safari doesnt support visitation of a URL by specifying it on the command line (e.g. safari.exe http://www.foo.com), the applications.conf mechanism could not be used, because it conveys information about which URL to visit via the command line. This necessitated the implementation of a custom plug-in, which sets the homepage (located on a file on disk) prior to opening safari. Because of this mechanism, safari can only visit one URL at a time.

5. Post installation
--------------------
Once the software is installed and configured it is time to create a new snapshot which we call a "safe state". This is the state that the machine is in perfect working order, i.e. no malicious programs are installed and the machine is considered to be clean. This state is reverted back to when a malicious event occurs inside of the virtual machine. When the virtual machine is running Windows XP and is fully booted, click on the Snapshot button in the VMware server to create a snapshot (May take some time so just leave it for a few minutes) 



