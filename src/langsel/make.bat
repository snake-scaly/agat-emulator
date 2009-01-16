call ..\config.bat
%RC% -fo.obj\resource.res langsel.rc
if errorlevel 1 exit
set SYSLIBS=user32.lib
set LIBS=..\dialog.c ..\resize.c ..\localize.c
%CLDEPS% %CFLAGS% -DUNICODE -D_UNICODE -Fo.obj\ -I.. -Felangsel.exe *.c %LIBS% %SYSLIBS% .obj\resource.res
