@echo off
for %%n in (cpu fdd joystick sound tape video .) do (
	echo Building %%n...
	pushd %%n
	call make.bat
	if errorlevel 1 exit
	popd
)
echo Complete.
