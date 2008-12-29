del emulator.exe
del emulator.ini
del interface.exe


rmdir saves
rmdir systems

rmdir /s /q .obj
rmdir /s /q cpu\.obj
rmdir /s /q fdd\.obj
rmdir /s /q joystick\.obj
rmdir /s /q sound\.obj
rmdir /s /q tape\.obj
rmdir /s /q video\.obj

del /s /q *.lib
