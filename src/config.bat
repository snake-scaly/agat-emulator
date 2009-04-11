set CFLAGS=-nologo -Ox -DKEY_SCANCODES -DDOUBLE_X -DDOUBLE_Y -D_CRT_SECURE_NO_DEPRECATE -DSYNC_SCREEN_UPDATE
set CLDEPS=cldeps.exe
set AR=lib.exe
set ARFLAGS=-nologo
set RC=rc.exe -DWINVER=0x600
if not exist .obj mkdir .obj
