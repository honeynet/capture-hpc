#!/bin/sh
echo $VIX_INCLUDE
gcc -I $VIX_INCLUDE -o revert revert.c $VIX_LIB/libvmware-vix.so
