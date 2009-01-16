for %%l in (english russian) do (
	rc /r /dSTANDALONE %%l.rh
	if errorlevel 1 exit
	link /nologo /dll /noentry /release /machine:x86 %%l.res
	if errorlevel 1 exit
	del %%l.res
)