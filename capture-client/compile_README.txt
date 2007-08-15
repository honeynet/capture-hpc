1. Capture Client Compilation Instructions
-----------------------
For convenient compilation, we have provided a makefile to compile the Capture Client.

2. Prerequisites
----------------
One needs the Windows 2003 WDK  (available at http://www.microsoft.com/whdc/devtools/WDK/default.mspx), the Windows Server 2003 Platform SDK (available at http://www.microsoft.com/downloads/details.aspx?FamilyId=A55B6B43-E24F-4EA3-A93E-40C0EC4F68E5&displaylang=en), the Boost C++ libraries (available at http://www.boost.org/)  and Visual C++ 2005 Express Edition (available at http://msdn.microsoft.com/vstudio/express/visualc/default.aspx) installed. 

3. Compilation
--------------
Prior to invoking the compilation, one needs to set the correct environment variables as follows:
1. Open up the Makefile inside the captureclient directory
	Change the inc and lib directories so they point to your Platform SDK, WDK, and Boost paths on your system
2. Open a command prompt
3. execute command <WINDOWS DIR>\system32\cmd.exe /k <WINDDK DIR>\bin\setenv.bat <WINDDK DIR> fre WXP
5. cd into the Microsoft Visual C++ Express directory and execute 'vcvarsall.bat'
6. cd into the captureclient directory and execute 'nmake'. 
6. After successful compilation, execute 'nmake install' to create a Capture Client release in the bin directory. Zip up the content of bin directory for the Capture release. Follow the instructions for installing the client described in README.txt.	
7. Before running the Capture client make sure you right click the FileMonitorInstallation.inf file and click install to install the filter driver