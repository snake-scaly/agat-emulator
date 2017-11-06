@echo off


set TDIR=..\..\..\tools
set ISCC=%TDIR%\ISCC.exe

if "%1"=="" goto nover

%ISCC% /dAppVer=%1 setup.iss
if errorlevel 1 goto err

echo Compilation successfull

goto end


:nover
echo Usage: %0 appver
goto end


:err
echo Compilation failed
goto end



:end
