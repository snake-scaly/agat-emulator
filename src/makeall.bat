@echo off
::call "F:\Program Files\Microsoft Visual Studio 9.0\VC\bin\vcvars32.bat" 
for %%n in (cpu fdd joystick sound tape video installio lang langsel softcard clock printer mouse ramcard cfgedit .) do (
	echo Building %%n...
	pushd %%n
	call make.bat
	if errorlevel 1 exit
	popd
)
echo Complete.
