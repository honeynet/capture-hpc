Capture Server README
---------------------
Capture allows to search and identify malicious servers on a network. Capture is split into two functional areas: a Capture Server and Capture Client. The primary purpose of the Capture Server is to control numerous Capture clients to interact with web servers. It allows to start and stop clients, instruct clients to interact with a web server retrieving a specified URI, and aggregating the classifications and supporting data of the Capture clients regards the web server they have interacted with. 

Capture is written and distributed under the GNU General Public License.

1.Prerequisites
---------------
* Sun's Java JRE 1.6.0 - update 7
* VMWare Server 1.0.6 with VMware VIX (do not download VIX separately) (available at http://www.vmware.com/download/server/) 
* Microsoft Windows XP, Microsoft Windows Vista or Linux (other OS might also be capable of running the server, but are not supported)

1.1 Installing the VMware VIX
-----------------------------
1. Install VMware server.

2. Capture Server Installation
------------------------------
Unpack the capture-server zip file.

3. Capture Server Configuration
-------------------------------
Configuring the server component requires editing the config.xml file that was distributed with the capture-server release.

* Open up the config.xml
* Configure the global options, such as the time that is allowed to pass to retrieve a URL, the option to automatically retrieve malware or network captures (on benign and malicious URLs), and the directive to push the local exclusion list to the clients. 
  Various timeout/delay options can also be configured via the global option (all in seconds):
  - client_inactivity_timeout: the capture client indicates that it is still alive via responding to a ping by the server. This happens every 10 seconds. If no pong is received by the client for the duration of the client_inactivity_timeout, the client inactivity error is thrown and the VM reverted. An example when this could happen is when a malicious site causes a blue screen.
  - revert_timeout: the vix code that the revert function makes use of, at times hangs, but functions properly if restarted. If the revert has not completed during the revert_timeout duration, the revert timeout error is thrown and the revert of the VM attempted once again.
  - vm_stalled_after_revert_timeout: identical to the revert_timeout, but the start criteria is not communicated by the VIX api, but rather by the capture client sending a visit command.
  - vm_stalled_during_operation_timeout: When client (e.g. Internet Explorer) locks up, the capture client is still able to respond to pings, but doesnt progress visitation of URLs. This vm_stalled_during_operation_timeout sets how often the capture server should at least expect a visitation event (this is highly dependent on speed of the network and how many URLs are being visited). If no visitation event is received during the timeout period, the VM stalled error is thrown and the VM is reverted.
  - same_vm_revert_delay: the vix library and vmware server have a difficult time reverting vms at the same time. the code already prevents the same VM from reverting at the same time. the delay specified by this variable is automatically applied when reverting the same vm.
  - different_vm_revert_delay: the vix library and vmware server have a difficult time reverting vms at the same time. the delay specified by this variable is automatically applied when reverting a different vm. This delay is larger because theoretically it would be possible to delay two VMs at the same time.
* The global option group size determines how many instances of the client application are opened at the same time. A value of 1 will cause only 1 instance to be opened (just like Capture-HPC v 2.01 and prior). Note only certain client plug-ins support visiting group of sizes larger than one:
	- internetexplorer (applies divide-and-conquer algorithm): full support (max group size of 80)
	- internetexplorerbulk (applies bulk algorithm): full support (max group size of 54)
	- Firefox (applies divide-and-conquer algorithm): full support; however, firefox needs to be configured to open a blank page and not restore from previous sessions. In addition, because firefox does not have a callback that notifies the server when a page has successfully been retrieved, the client-default-visit-time needs to be increased to accommodate loading X firefox instances and retrieving the web pages. Some testing might be required to determine the appropriate value.
	- Other: only group sizes of 1 are supported at this point
* Add the local exclusion lists that would be pushed to the clients if that option is enabled 
* Add vmware servers
	Specify the ip address, port, username, and password of the vmware server that hosts capture clients.
* For each vmware server, add virtual machines that run a Capture Client.
	Specify the path to the virtual machine vmx file as well as the administrator account and password and path the capture bat file exists (needs to be a bat file).

Example:
<config xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
        xsi:noNamespaceSchemaLocation="config.xsd">
	<global collect-modified-files="false" 
			client-default="iexplore"
			client-default-visit-time="10"
			capture-network-packets-malicious="false"
			capture-network-packets-benign="false"
			send-exclusion-lists="false"
        		group_size="50"
				vm_stalled_after_revert_timeout="120"
            	revert_timeout="120"
            	client_inactivity_timeout="60"
            	vm_stalled_during_operation_timeout="300"
            	same_vm_revert_delay="6"
            	different_vm_revert_delay="24"
	/>

        <exclusion-list monitor="file" file="FileMonitor.exl" />
        <exclusion-list monitor="process" file="ProcessMonitor.exl" />
        <exclusion-list monitor="registry" file="RegistryMonitor.exl" />

	<virtual-machine-server type="vmware-server" address="192.168.1.1" port="902" 
		username="Username" password="password">
		<virtual-machine vm-path="C:\Virtual Machines\winxp\winxp.vmx" 
						 client-path="C:\Progra~1\capture\CaptureClient.bat" 
						 username="Administrator" 
						 password="admin_pw"/>
	</virtual-machine-server>
</config>

The config file above specifies one vmware server with one virtual machine that should be managed by capture.

4 Capture Server Usage
----------------------
The Capture server allows one to specify a list of uris for the clients to visit as well as push local exclusion lists to the clients. The Capture server automatically starts the virtual machines on the vmware servers specified in the configuration file and starts to distribute the specified uris in round robin fashion to the Capture clients. 

In order to start the server one needs to specify a list of URIs that should be visited by the clients. This happens by starting the server with the command line option -f input_uris.txt that contains a list of URIs, where a URI is placed onto each line. For example, one can create a file uris.txt of URI's like:
http://www.google.com
http://www.yahoo.com
Example: java -Djava.net.preferIPv4Stack=true -jar CaptureServer.jar -s <IP listening address>:<IP listening port> -f input_uris.txt.

One can specify a specific client application to have Capture client to visit a server with (The default is set via the client-default global property in the config.xml. By default it is set to Internet Explorer Bulk). This occurs by appending a client identifier separated by two colons after the URI. Also one can overwrite the default visitation time, for example, http://www.google.com::firefox::45. The client identifier needs to be specified in the applications.conf on the client side and point to the executable of the client application. When group size is configured to be larger than 1, it is not recommended to overwrite the visitiation time and client. (see the Capture Client readme.txt for more information)

4.3 Report Description
----------------------
As the Capture clients interact with potentially malicious servers, log files/directories are being created that convey information about which URI's have been visited and the classification of the visited URI's. If a URI is classified as malicious, additional information about the state changes that occured on the client are logged, and, if enabled, the malware and network captures are being downloaded onto the server. 

* safe.log - safe.log contains the list of uris that have been visited and are deemed benign. 
* progress.log - progress.log contains information about which URIs are currently being visited or have been visited. If an error occurs during visitation of URI, it will be logged in this log file as well. The server will attempt to visit a URI 5 times before it gives up. Once this occurs, an error is logged in the error.log
* error.log - error.log contains information about URIs that could not be visited.
* stats.log - contains information about the performance of the capture system (ie how many URLs are visited, how many reverts took place, how long does a revert take on avg, etc.) This can be used to fine tune capture and select the appropriate hardware for the capture system.
* malicious.log - malicious.log contains the list of uris that have been visited and are deemed malicious. 
* server_timestamp.log - server_timestamp.log (e.g. http%3A%2F%2Fwww.google.com_20070101_180000.log) is created for each URI that is deemed malcious. It contains a list of the state changes that occured. 
* server_timestamp.zip - this is the file that contains the files that have been modified or deleted off the client machine during the interaction with a malicious servers. It also contains (if enabled) the network dump recorded on the machine.
