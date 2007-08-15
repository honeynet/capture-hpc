##############################################################################
##
##  Capture-HPC
##


## Change these paths to the correct paths on your system
BOOSTINC = C:\Boost\include\boost-1_33_1
BOOSTLIB = C:\Boost\lib
PLATFORMSDKINC = C:\Program Files\Microsoft Platform SDK\Include
PLATFORMSDKLIB = C:\Program Files\Microsoft Platform SDK\Lib
WDKINC = C:\WinDDK\6000\inc\api
WDKLIB = C:\WinDDK\6000\lib\wxp\i386

BIND = $(MAKEDIR)\bin


INCLUDE = $(INCLUDE);$(BOOSTINC);$(PLATFORMSDKINC);$(WDKINC)
LIB = $(LIB);$(BOOSTLIB);$(PLATFORMSDKLIB);$(WDKLIB)

all: dirs \
	KernelDriver \
	Capture \

dirs:
	if not exist $(BIND) mkdir $(BIND)

KernelDriver :


Capture :
	@vcbuild Capture.vcproj Release /u
	cd "$(MAKEDIR)"
	cd "Clients"
	@vcbuild Client_InternetExplorer.vcproj Release /u
	@vcbuild Client_MicrosoftOffice.vcproj Release /u
	@vcbuild Client_OpenOffice.vcproj Release /u
	cd "$(MAKEDIR)"
	cd "KernelDriver"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)"

install :
	cd "$(MAKEDIR)\KernelDriver"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) install
	cd "$(MAKEDIR)"
	copy "Readme.txt" $(BIND)
	copy "COPYING" $(BIND)
	copy "Capture.bat" $(BIND)
	cd "release"
	copy "Capture.exe" $(BIND)
	cd "$(MAKEDIR)"
	cd "ExclusionLists"
	copy *.exl $(BIND)
	cd "$(MAKEDIR)"
	copy *.inf $(BIND)
	cd "$(MAKEDIR)"
	cd "Config"
	copy *.conf $(BIND)
	cd "$(MAKEDIR)"
	cd "ext_lib"
	copy *.* $(BIND)
	cd "$(MAKEDIR)"
	cd "Clients"
	cd "release"
	if not exist "$(BIND)\plugins" mkdir "$(BIND)\plugins"
	copy *.dll "$(BIND)\plugins\"
	cd "$(MAKEDIR)"


clean:
	cd "$(MAKEDIR)\KernelDriver"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)"
	@vcbuild Capture.vcproj /c Release
	-del /Q /S $(BIND) 2>nul