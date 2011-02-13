copy /b interface.exe emulator.exe
editbin /subsystem:windows /release emulator.exe
upx -9 --compress-icons=0 emulator.exe
copy /b emulator.exe ..\release
copy /b lang\*.dll ..\release\lang
copy /b installio\installio.exe ..\release\drivers
copy /b langsel\langsel.exe ..\release
copy /b cfgedit\cfgedit.exe ..\release
del ..\release\emulator.ini
