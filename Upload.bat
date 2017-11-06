@echo off

set TDIR=..\..\tools
set PSCP=%TDIR%\pscp.exe
set PLINK=%TDIR%\plink.exe

if "%1"=="" goto nover
if "%2"=="" goto nouser
if "%3"=="" goto nopass

rmdir /s /q .tmp
mkdir .tmp
mkdir .tmp\%1
copy /b agatemulator-%1.exe .tmp\%1
if errorlevel 1 goto err
copy /b release-%1-en.txt .tmp\%1
if errorlevel 1 goto err
copy /b release-%1-ru.txt .tmp\%1
if errorlevel 1 goto err
%PSCP% -r -pw %3 .tmp\%1 %2@web.sourceforge.net:/home/frs/project/agatemulator/agatemulator
if errorlevel 1 goto err

rmdir /s /q .tmp

echo Upload successfull

goto end


:nover
:nouser
:nopas
echo Usage: %0 appver user pass
goto end


:err
echo Upload failed
goto end



:end
