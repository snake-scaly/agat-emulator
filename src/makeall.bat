@echo off
for %%n in (cpu fdd joystick sound tape video installio lang langsel softcard clock printer ramcard .) do (
	echo Building %%n...
	pushd %%n
	call make.bat
	if errorlevel 1 exit
	popd
)
echo Complete.
