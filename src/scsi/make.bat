call ..\config.bat
%CLDEPS% %CFLAGS% -c -Fo.obj\ -I.. -I../.. *.c
if errorlevel 1 exit
%AR% %ARFLAGS% -out:libscsi.lib .obj\*.obj
if errorlevel 1 exit
