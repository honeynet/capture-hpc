#!/bin/sh
echo $VIX_INCLUDE
gcc -I $JAVA_HOME/include -I $JAVA_HOME/include/linux -I $VIX_INCLUDE VMwareServerExt.c -c -g
gcc -shared $VIX_LIB/libvmware-vix.so VMwareServerExt.o -o libVMwareServerExt.so -ldl
