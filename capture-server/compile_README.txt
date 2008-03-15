1. Capture Client Compilation Instructions
------------------------------------------
For convenient compilation, we provide an Ant script for easy compilation of the Capture Server.

2. Prerequisites
----------------
One needs Apache Ant 1.7.0 (available at http://ant.apache.org) with Ant Contrib 1.0 (available at http://ant-contrib.sourceforge.net/), Sun's Java JDK 1.6.0_02 (available at http://sun.java.com), the VMware VIX (part of VMware Server 1.0.3) (available from http://www.vmware.com) and a C compiler. We use Microsoft Visual C++ Express 2005 (available from http://msdn.microsoft.com/vstudio/express/downloads/) on our Windows build environment and GCC on the Linux build environment. 

Ensure that the following environment variables are set:
	JAVA_HOME
	VIX_LIB
	VIX_INCLUDE
	VIX_HOME
	ANT_HOME
	
Ensure that the following directories are in the PATH:
	JAVA_HOME/bin
	ANT_HOME/bin
	VCINSTALLDIR/bin for Windows
	path to gcc for Linux

3. Compilation
--------------
Open a Visual Studio 2005 Command Prompt on Windows/ regular terminal windows on Linux and cd into directory of the Capture Server source. Type ant and press enter. This script will:
	- compile the java source and package the class files into a jar
	- compile the revert application
	- copy the relevant files into the release directory.
	- package the compiled server files into CaptureServer-Release.zip

The uncompressed release will be located in the release directory. Please refer to the Readme.txt file on how to configure and run the server. 
