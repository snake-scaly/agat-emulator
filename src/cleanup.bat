del emulator.exe
del emulator.ini
del interface.exe
del installio\installio.exe
del langsel\langsel.exe
del lang\*.dll


rmdir saves
rmdir systems

rmdir /s /q .obj
rmdir /s /q cpu\.obj
rmdir /s /q fdd\.obj
rmdir /s /q joystick\.obj
rmdir /s /q sound\.obj
rmdir /s /q tape\.obj
rmdir /s /q video\.obj
rmdir /s /q softcard\.obj
rmdir /s /q clock\.obj
del /q installio\*.obj
rmdir /s /q langsel\.obj

del /s /q *.lib
