call config.bat
%RC% -fo.obj\resource.res resource.rc
if errorlevel 1 exit
set SYSLIBS=comctl32.lib user32.lib gdi32.lib comdlg32.lib shlwapi.lib htmlhelp.lib advapi32.lib
set LIBS=video\libvideo.lib cpu\libcpu.lib sound\libsound.lib tape\libtape.lib joystick\libjoystick.lib fdd\libfdd.lib softcard\libsoftcard.lib clock\libclock.lib printer\libprinter.lib ramcard\libramcard.lib mouse\libmouse.lib
%CLDEPS% %CFLAGS% -Fo.obj\ -I.. *.c -Feinterface.exe .obj\resource.res %LIBS% %SYSLIBS%
