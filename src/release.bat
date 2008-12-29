copy /b interface.exe emulator.exe
editbin /subsystem:windows /release emulator.exe
upx -9 emulator.exe
