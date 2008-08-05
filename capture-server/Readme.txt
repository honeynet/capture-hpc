Capture Server README
---------------------
Capture allows to search and identify malicious servers on a network. Capture is split into two functional areas: a Capture Server and Capture Client. The primary purpose of the Capture Server is to control numerous Capture clients to interact with web servers. It allows to start and stop clients, instruct clients to interact with a web server retrieving a specified URI, and aggregating the classifications and supporting data of the Capture clients regards the web server they have interacted with. 

Capture is written and distributed under the GNU General Public License.

1.Prerequisites
---------------
* Sun's Java JRE 1.6.0_02
* VMWare Server 1.0.5 with VMware VIX (available at http://www.vmware.com/download/server/) 
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
* Configure the global options, such as the time that is allowed to pass to retrieve a URL, the option to automatically retrieve malware or network captures (on benign and malicious URLs), and the directive to push the local exclusion list to the clients. The timeout_factor allows one to increase the various timeouts (e.g. the number of seconds it takes before a VM is reverted after client fails to connect to server after a revert)
* The value p_m turns on the divide&conquer feature (see http://www.mcs.vuw.ac.nz/~cseifert/publications/cseifert_divide_and_conquer.pdf) in which client applications are started in parallel and if a malicious state change is detected, the set is divided in half and iterativly visited until the malicious URL is identified (Note there are some risks of missing attacks using this feature; in particular attacks that make use of ip tracking functionality. The paper http://www.mcs.vuw.ac.nz/~cseifert/publications/IFIP2008_CSeifert_Paper66.pdf describes the setup that can reduce this risk.
The global option group size determines how many instances of the client application are opened at the same time. The lower the value, the more instances are opened. A value of 1 will cause only 1 instance to be opened (just like Capture-HPC v 2.01 and prior). A value of 0.004 will cause 80 (max) instances to be opened. Note only certain client applications support this feature:
	- IE: full support
	- Firefox: full support; however, firefox needs to be configured to open a blank page and not restore from previous sessions. In addition, because firefox does not have a callback that notifies the server when a page has successfully been retrieved, the client-default-visit-time needs to be increased to accommodate loading X firefox instances and retrieving the web pages. Some testing might be required to determine the appropriate value.
	- Other: not supported at this point
* Add the local exclusion lists that would be pushed to the clients if that option is enabled 
* Add vmware servers
	Specify the ip address, port, username, and password of the vmware server that hosts capture clients.
* For each vmware server, add virtual machines that run a Capture Client.
	Specify the path to the virtual machine vmx file as well as the administrator account and password and path the capture bat file exists (needs to be a bat file).

Example:
<config xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
        xsi:noNamespaceSchemaLocation="config.xsd">
	<global collect-modified-files="false" 
			client-default-visit-time="10"
			capture-network-packets-malicious="false"
			capture-network-packets-benign="false"
			send-exclusion-lists="false"
        		group_size="50"
			timeout_factor="1.0"
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

One can specify a specific client application to have Capture client to visit a server with (The default is Internet Explorer). This occurs by appending a client idenifier separated by two colons after the URI. Also one can overwrite the default visitation time, for example, http://www.google.com::firefox::45. The client identifier needs to be specified in the applications.conf on the client side and point to the executable of the client application. (see the Capture Client readme.txt for more information)

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
