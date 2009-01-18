call ..\config.bat
%CLDEPS% %CFLAGS% -c -Fo.obj\ -I.. -I../.. z80.c
if errorlevel 1 exit
%AR% %ARFLAGS% -out:libsoftcard.lib .obj\*.obj
if errorlevel 1 exit
