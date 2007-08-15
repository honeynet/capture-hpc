##############################################################################
##
##  Capture-HPC
##

BIND = ..\..\..\bin\

all: KernelDriver \

KernelDriver :
	cd "FileMonitor"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)"
	cd "RegistryMonitor"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)"
	cd "ProcessMonitor"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)"

install :
	cd "FileMonitor\i386"
	copy *.sys $(BIND)
	cd "$(MAKEDIR)"
	cd "RegistryMonitor\i386"
	copy *.sys $(BIND)
	cd "$(MAKEDIR)"
	cd "ProcessMonitor\i386"
	copy *.sys $(BIND)
	cd "$(MAKEDIR)"

clean:
	cd "FileMonitor"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)"
	cd "RegistryMonitor"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)"
	cd "ProcessMonitor"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)"