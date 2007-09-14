cl -I "%JAVA_HOME%\include" -I "%JAVA_HOME%\include\win32" -I "%VIX_INCLUDE%" VMwareServerExt.c /LD /link "%VIX_LIB%\vix.lib"
