call ..\config.bat
%CLDEPS% %CFLAGS% -c -Fo.obj\ -I.. -I../.. fdd.c fdd1.c fddaa.c
if errorlevel 1 exit
%CLDEPS% %CFLAGS% -Fo.obj1\ -I.. -I../.. fdd1extr.c
if errorlevel 1 exit
%CLDEPS% %CFLAGS% -Fo.obj1\ -I.. -I../.. genimage.c
if errorlevel 1 exit
%AR% %ARFLAGS% -out:libfdd.lib .obj\*.obj
if errorlevel 1 exit
