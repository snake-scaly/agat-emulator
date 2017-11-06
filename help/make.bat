@echo off
set TDIR=..\..\..\tools
set HHC=%TDIR%\hhc.exe

%HHC% russian.hhp
%HHC% english.hhp
