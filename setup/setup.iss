#ifndef AppVer
#error "Define AppVer by compiler command line /d option"
#endif

//#define AppVer "1.28"


[Setup]
AppName=Agat Emulator
AppId=agatemul
AppVerName=Agat Emulator – {#AppVer}
DefaultDirName={pf}\Agat
DefaultGroupName=Agat Emulator
Compression=lzma
SolidCompression=yes
OutputDir=.
OutputBaseFilename=agatemulator-{#AppVer}


[Languages]
Name: ru; MessagesFile: compiler:Languages\Russian.isl; InfobeforeFile:"../release-{#AppVer}-ru.txt"
Name: en; MessagesFile: compiler:Default.isl; InfobeforeFile:"../release-{#AppVer}-en.txt"


[CustomMessages]
en.register_dsk=Register .DSK images to:
ru.register_dsk=Зарегистрировать расширение .DSK на:
en.register_nib=Register .NIB images to:
ru.register_nib=Зарегистрировать расширение .NIB на:
en.register_aim=Register .AIM images to:
ru.register_aim=Зарегистрировать расширение .AIM на:
en.register_po=Register .PO images to:
ru.register_po=Зарегистрировать расширение .PO на:
en.register_do=Register .DO images to:
ru.register_do=Зарегистрировать расширение .DO на:
en.register_hdv=Register .HDV images to:
ru.register_hdv=Зарегистрировать расширение .HDV на:
en.register_40t=Register .40T images to:
ru.register_40t=Зарегистрировать расширение .40T на:
en.register_pds=Register .PDS images to:
ru.register_pds=Зарегистрировать расширение .PDS на:

en.reg_A7=Agat-7 system
ru.reg_A7=систему Агат-7
en.reg_A7_140=Agat-7 system (140K)
ru.reg_A7_140=систему Агат-7 (140K)
en.reg_A9=Agat-9 system
ru.reg_A9=систему Агат-9
en.reg_A9_140=Agat-9 system (140K)
ru.reg_A9_140=систему Агат-9 (140K)
en.reg_A2=Apple ][ system
ru.reg_A2=систему Apple ][
en.reg_A2e=Apple //e system
ru.reg_A2e=систему Apple //e
en.reg_atom=Acorn Atom system
ru.reg_atom=систему Acorn Atom
en.reg_pravetz=Pravetz system
ru.reg_pravetz=систему Правец

en.agat_dsk_info=Agat Disk Image
ru.agat_dsk_info=Образ диска ПЭВМ Агат
en.apple_dsk_info=Apple ][ Disk Image
ru.apple_dsk_info=Образ диска Apple ][

en.apple_hdv_info=Apple ][ Hard Disk Image
ru.apple_hdv_info=Образ жёсткого диска Apple ][

en.atom_40t_info=Acorn Atom Disk Image
ru.atom_40t_info=Образ диска Acorn Atom

en.pravetz_disk_info=Pravetz Disk Image
ru.pravetz_disk_info=Образ диска Правец

en.open_A7=Open for Agat-7
ru.open_A7=Открыть для Агат-7
en.open_A9=Open for Agat-9
ru.open_A9=Открыть для Агат-9
en.open_A2=Open for Apple ][
ru.open_A2=Открыть для Apple ][
en.open_A2e=Open for Apple //e
ru.open_A2e=Открыть для Apple //e
en.open_atom=Open for Acorn Atom
ru.open_atom=Открыть для Acorn Atom
en.open_pravetz=Open for Pravetz
ru.open_pravetz=Открыть для Правец


[Files]
Source: ..\release\*; DestDir: {app}; Flags: recursesubdirs ignoreversion
Source: ..\release\drivers\giveio.sys; DestDir: {sys}\drivers; Flags: sharedfile; Tasks: installdriver; MinVersion: 0, 4.0



[Icons]
Name: {group}\Эмулятор Агат; Filename: {app}\emulator.exe; WorkingDir: {app}; Languages: ru
Name: {group}\Agat Emulator; Filename: {app}\emulator.exe; WorkingDir: {app}; Languages: en
Name: {group}\Выбор языка; Filename: {app}\langsel.exe; WorkingDir: {app}; Languages: ru
Name: {group}\Language Selection; Filename: {app}\langsel.exe; WorkingDir: {app}; Languages: en
Name: {group}\Удаление эмулятора; Filename: {uninstallexe}; WorkingDir: {app}; Languages: ru
Name: {group}\Remove Emulator; Filename: {uninstallexe}; WorkingDir: {app}; Languages: en
Name: {userdesktop}\Эмулятор Агат; Filename: {app}\emulator.exe; WorkingDir: {app}; Tasks: desktopicon; Languages: ru
Name: {userdesktop}\Agat Emulator; Filename: {app}\emulator.exe; WorkingDir: {app}; Tasks: desktopicon; Languages: en

[Tasks]
Name: desktopicon; Description: {cm:CreateDesktopIcon}

Name: register_dsk; Description: {cm:register_dsk}
Name: register_dsk/A7; Description: {cm:reg_A7}; Flags: exclusive unchecked;
Name: register_dsk/A9; Description: {cm:reg_A9}; Languages: ru; Flags: exclusive
Name: register_dsk/A9; Description: {cm:reg_A9}; Languages: en; Flags: exclusive unchecked
Name: register_dsk/A2; Description: {cm:reg_A2}; Flags: exclusive unchecked;
Name: register_dsk/A2e; Description: {cm:reg_A2e}; Languages: ru; Flags: exclusive unchecked
Name: register_dsk/A2e; Description: {cm:reg_A2e}; Languages: en; Flags: exclusive

Name: register_nib; Description: {cm:register_nib}
Name: register_nib/A7; Description: {cm:reg_A7}; Flags: exclusive unchecked;
Name: register_nib/A9; Description: {cm:reg_A9}; Flags: exclusive; Languages: ru
Name: register_nib/A9; Description: {cm:reg_A9}; Flags: exclusive unchecked; Languages: en
Name: register_nib/A2; Description: {cm:reg_A2}; Flags: exclusive unchecked;
Name: register_nib/A2e; Description: {cm:reg_A2e}; Flags: exclusive unchecked; Languages: ru
Name: register_nib/A2e; Description: {cm:reg_A2e}; Flags: exclusive; Languages: en

Name: register_aim; Description: {cm:register_aim}
Name: register_aim/A7; Description: {cm:reg_A7}; Flags: exclusive unchecked;
Name: register_aim/A9; Description: {cm:reg_A9}; Flags: exclusive

Name: register_do; Description: {cm:register_do}
Name: register_do/A2; Description: {cm:reg_A2}; Flags: exclusive unchecked;
Name: register_do/A2e; Description: {cm:reg_A2e}; Flags: exclusive

Name: register_po; Description: {cm:register_po}
Name: register_po/A2; Description: {cm:reg_A2}; Flags: exclusive unchecked;
Name: register_po/A2e; Description: {cm:reg_A2e}; Flags: exclusive

Name: register_hdv; Description: {cm:register_hdv}
Name: register_hdv/A2e; Description: {cm:reg_A2e}; Flags: exclusive

Name: register_40t; Description: {cm:register_40t}
Name: register_40t/atom; Description: {cm:reg_atom}; Flags: exclusive

Name: register_pds; Description: {cm:register_pds}
Name: register_pds/pravetz; Description: {cm:reg_pravetz}; Flags: exclusive

Name: installdriver; Description: Установка драйвера для прямого доступа к динамику; Flags: unchecked; MinVersion: 0, 4.0; Languages: ru
Name: installdriver; Description: Install driver to access PC Speaker; Flags: unchecked; MinVersion: 0, 4.0; Languages: en

[Run]
Filename: {app}\drivers\installio.exe; Parameters: "/i"; Tasks: installdriver; Description: Установка драйвера прямого доступа; Flags: runhidden; Languages: ru
Filename: {app}\drivers\installio.exe; Parameters: "/i"; Tasks: installdriver; Description: Installing driver; Flags: runhidden; Languages: en
;Filename: {src}\readme.txt; Description: Показать файл Readme после установки; Flags: postinstall shellexec skipifsilent

[UninstallRun]
Filename: {app}\drivers\installio.exe; Parameters: "/u"; Tasks: installdriver; Flags: runhidden


[Registry]
Root: HKLM; Subkey: Software\Agat; ValueType: string; ValueName: instdir; ValueData: {app}; Flags: uninsdeletekey
Root: HKLM; Subkey: Software\Agat; ValueType: string; ValueName: version; ValueData: {#AppVer}; Flags: uninsdeletekey
Root: HKLM; Subkey: Software\Agat; ValueType: string; ValueName: grpdir; ValueData: {group}; Flags: uninsdeletekey
Root: HKLM; Subkey: Software\Agat; ValueType: string; ValueName: prjname; ValueData: Эмулятор Агат; Flags: uninsdeletekey; Languages: ru
Root: HKLM; Subkey: Software\Agat; ValueType: string; ValueName: prjname; ValueData: Agat Emulator; Flags: uninsdeletekey; Languages: en

Root: HKCU; Subkey: Software\Agat; ValueType: string; ValueName: sysdir; ValueData: {app}\systems; Flags: uninsdeletekey
Root: HKCU; Subkey: Software\Agat; ValueType: string; ValueName: savdir; ValueData: {app}\saves; Flags: uninsdeletekey
Root: HKCU; Subkey: Software\Agat; ValueType: string; ValueName: romdir; ValueData: {app}\roms; Flags: uninsdeletekey
Root: HKCU; Subkey: Software\Agat; ValueType: string; ValueName: fntdir; ValueData: {app}\fnts; Flags: uninsdeletekey
Root: HKCU; Subkey: Software\Agat; ValueType: string; ValueName: dskdir; ValueData: {app}\disks; Flags: uninsdeletekey
Root: HKCU; Subkey: Software\Agat; ValueType: string; ValueName: tapdir; ValueData: {app}\tapes; Flags: uninsdeletekey
Root: HKCU; Subkey: Software\Agat; ValueType: string; ValueName: lngdir; ValueData: {app}\lang; Flags: uninsdeletekey

Root: HKCR; Subkey: "Apple2Image"; ValueType: string; ValueName: ""; ValueData: {cm:apple_dsk_info}; Flags: uninsdeletekey
Root: HKCR; Subkey: "Apple2Image\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\emulator.exe,1"
Root: HKCR; Subkey: "Apple2Image\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(3) Apple ][ Plus"" -cd ""{app}"" -s6d1 ""%1"""
Root: HKCR; Subkey: "Apple2Image\shell\open_agat7\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(1) IKP-7"" -cd ""{app}"" -s3d1 ""%1"""
Root: HKCR; Subkey: "Apple2Image\shell\open_agat9\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(2) IKP-9"" -cd ""{app}"" -s5d1 ""%1"""
Root: HKCR; Subkey: "Apple2Image\shell\open_apple2e\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(8b) Apple 2e Test"" -cd ""{app}"" -s6d1 ""%1"""
Root: HKCR; Subkey: "Apple2Image\shell\open"; ValueType: string; ValueName: ""; ValueData: {cm:open_A2}
Root: HKCR; Subkey: "Apple2Image\shell\open_apple2e"; ValueType: string; ValueName: ""; ValueData: {cm:open_A2e}
Root: HKCR; Subkey: "Apple2Image\shell\open_agat7"; ValueType: string; ValueName: ""; ValueData: {cm:open_A7}
Root: HKCR; Subkey: "Apple2Image\shell\open_agat9"; ValueType: string; ValueName: ""; ValueData: {cm:open_A9}

Root: HKCR; Subkey: "Apple2eImage"; ValueType: string; ValueName: ""; ValueData: {cm:apple_dsk_info}; Flags: uninsdeletekey
Root: HKCR; Subkey: "Apple2eImage\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\emulator.exe,1"
Root: HKCR; Subkey: "Apple2eImage\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(8b) Apple 2e Test"" -cd ""{app}"" -s6d1 ""%1"""
Root: HKCR; Subkey: "Apple2eImage\shell\open_agat7\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(1) IKP-7"" -cd ""{app}"" -s3d1 ""%1"""
Root: HKCR; Subkey: "Apple2eImage\shell\open_agat9\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(2) IKP-9"" -cd ""{app}"" -s5d1 ""%1"""
Root: HKCR; Subkey: "Apple2eImage\shell\open_apple2\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(3) Apple ][ Plus"" -cd ""{app}"" -s6d1 ""%1"""
Root: HKCR; Subkey: "Apple2eImage\shell\open"; ValueType: string; ValueName: ""; ValueData: {cm:open_A2e}
Root: HKCR; Subkey: "Apple2eImage\shell\open_apple2"; ValueType: string; ValueName: ""; ValueData: {cm:open_A2}
Root: HKCR; Subkey: "Apple2eImage\shell\open_agat7"; ValueType: string; ValueName: ""; ValueData: {cm:open_A7}
Root: HKCR; Subkey: "Apple2eImage\shell\open_agat9"; ValueType: string; ValueName: ""; ValueData: {cm:open_A9}

Root: HKCR; Subkey: "AppleHDImage"; ValueType: string; ValueName: ""; ValueData: {cm:apple_hdv_info}; Flags: uninsdeletekey
Root: HKCR; Subkey: "AppleHDImage\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\emulator.exe,2"
Root: HKCR; Subkey: "AppleHDImage\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(3j) Apple Desktop"" -cd ""{app}"" -s7d0 ""%1"""
Root: HKCR; Subkey: "AppleHDImage\shell\open"; ValueType: string; ValueName: ""; ValueData: {cm:open_A2}

Root: HKCR; Subkey: "Atom40tImage"; ValueType: string; ValueName: ""; ValueData: {cm:atom_40t_info}; Flags: uninsdeletekey
Root: HKCR; Subkey: "Atom40tImage\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\emulator.exe,1"
Root: HKCR; Subkey: "Atom40tImage\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(9a) Acorn Atom"" -cd ""{app}"" -fdd1 ""%1"""
Root: HKCR; Subkey: "Atom40tImage\shell\open"; ValueType: string; ValueName: ""; ValueData: {cm:open_atom}

Root: HKCR; Subkey: "Agat7Image"; ValueType: string; ValueName: ""; ValueData: {cm:agat_dsk_info}; Flags: uninsdeletekey
Root: HKCR; Subkey: "Agat7Image\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\emulator.exe,1"
Root: HKCR; Subkey: "Agat7Image\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(1) IKP-7"" -cd ""{app}"" -s3d1 ""%1"""
Root: HKCR; Subkey: "Agat7Image\shell\open_agat9\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(2) IKP-9"" -cd ""{app}"" -s5d1 ""%1"""
Root: HKCR; Subkey: "Agat7Image\shell\open_apple2\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(3) Apple ][ Plus"" -cd ""{app}"" -s6d1 ""%1"""
Root: HKCR; Subkey: "Agat7Image\shell\open_apple2e\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(8b) Apple 2e Test"" -cd ""{app}"" -s6d1 ""%1"""
Root: HKCR; Subkey: "Agat7Image\shell\open"; ValueType: string; ValueName: ""; ValueData: {cm:open_A7}
Root: HKCR; Subkey: "Agat7Image\shell\open_agat9"; ValueType: string; ValueName: ""; ValueData: {cm:open_A9}
Root: HKCR; Subkey: "Agat7Image\shell\open_apple2"; ValueType: string; ValueName: ""; ValueData:  {cm:open_A2}
Root: HKCR; Subkey: "Agat7Image\shell\open_apple2e"; ValueType: string; ValueName: ""; ValueData: {cm:open_A2e}


Root: HKCR; Subkey: "Agat9Image"; ValueType: string; ValueName: ""; ValueData: {cm:agat_dsk_info}; Flags: uninsdeletekey
Root: HKCR; Subkey: "Agat9Image\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\emulator.exe,1"
Root: HKCR; Subkey: "Agat9Image\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(2) IKP-9"" -cd ""{app}"" -s5d1 ""%1"""
Root: HKCR; Subkey: "Agat9Image\shell\open_agat7\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(1) IKP-7"" -cd ""{app}"" -s3d1 ""%1"""
Root: HKCR; Subkey: "Agat9Image\shell\open_apple2\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(3) Apple ][ Plus"" -cd ""{app}"" -s6d1 ""%1"""
Root: HKCR; Subkey: "Agat9Image\shell\open_apple2e\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(8b) Apple 2e Test"" -cd ""{app}"" -s6d1 ""%1"""
Root: HKCR; Subkey: "Agat9Image\shell\open"; ValueType: string; ValueName: ""; ValueData: {cm:open_A9}
Root: HKCR; Subkey: "Agat9Image\shell\open_agat7"; ValueType: string; ValueName: ""; ValueData: {cm:open_A7}
Root: HKCR; Subkey: "Agat9Image\shell\open_apple2"; ValueType: string; ValueName: ""; ValueData: {cm:open_A2}
Root: HKCR; Subkey: "Agat9Image\shell\open_apple2e"; ValueType: string; ValueName: ""; ValueData: {cm:open_A2e}


Root: HKCR; Subkey: "PravetzImage"; ValueType: string; ValueName: ""; ValueData: {cm:agat_dsk_info}; Flags: uninsdeletekey
Root: HKCR; Subkey: "PravetzImage\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\emulator.exe,1"
Root: HKCR; Subkey: "PravetzImage\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(3a) Pravetz 8A + Paris"" -cd ""{app}"" -s6d1 ""%1"""
Root: HKCR; Subkey: "PravetzImage\shell\open"; ValueType: string; ValueName: ""; ValueData: {cm:open_pravetz}


Root: HKCR; Subkey: ".dsk"; ValueType: string; ValueName: ""; ValueData: "Apple2Image"; Flags: uninsdeletevalue; Tasks: register_dsk/A2
Root: HKCR; Subkey: ".dsk"; ValueType: string; ValueName: ""; ValueData: "Apple2eImage"; Flags: uninsdeletevalue; Tasks: register_dsk/A2e
Root: HKCR; Subkey: ".dsk"; ValueType: string; ValueName: ""; ValueData: "Agat7Image"; Flags: uninsdeletevalue; Tasks: register_dsk/A7
Root: HKCR; Subkey: ".dsk"; ValueType: string; ValueName: ""; ValueData: "Agat9Image"; Flags: uninsdeletevalue; Tasks: register_dsk/A9

Root: HKCR; Subkey: ".nib"; ValueType: string; ValueName: ""; ValueData: "Apple2Image"; Flags: uninsdeletevalue; Tasks: register_nib/A2
Root: HKCR; Subkey: ".nib"; ValueType: string; ValueName: ""; ValueData: "Apple2eImage"; Flags: uninsdeletevalue; Tasks: register_nib/A2e
Root: HKCR; Subkey: ".nib"; ValueType: string; ValueName: ""; ValueData: "Agat7Image"; Flags: uninsdeletevalue; Tasks: register_nib/A7
Root: HKCR; Subkey: ".nib"; ValueType: string; ValueName: ""; ValueData: "Agat9Image"; Flags: uninsdeletevalue; Tasks: register_nib/A9

Root: HKCR; Subkey: ".aim"; ValueType: string; ValueName: ""; ValueData: "Agat7Image"; Flags: uninsdeletevalue; Tasks: register_aim/A7
Root: HKCR; Subkey: ".aim"; ValueType: string; ValueName: ""; ValueData: "Agat9Image"; Flags: uninsdeletevalue; Tasks: register_aim/A9

Root: HKCR; Subkey: ".po"; ValueType: string; ValueName: ""; ValueData: "Apple2Image"; Flags: uninsdeletevalue; Tasks: register_po/A2
Root: HKCR; Subkey: ".po"; ValueType: string; ValueName: ""; ValueData: "Apple2eImage"; Flags: uninsdeletevalue; Tasks: register_po/A2e

Root: HKCR; Subkey: ".do"; ValueType: string; ValueName: ""; ValueData: "Apple2Image"; Flags: uninsdeletevalue; Tasks: register_do/A2
Root: HKCR; Subkey: ".do"; ValueType: string; ValueName: ""; ValueData: "Apple2eImage"; Flags: uninsdeletevalue; Tasks: register_do/A2e

Root: HKCR; Subkey: ".hdv"; ValueType: string; ValueName: ""; ValueData: "AppleHDImage"; Flags: uninsdeletevalue; Tasks: register_hdv/A2e

Root: HKCR; Subkey: ".40t"; ValueType: string; ValueName: ""; ValueData: "Atom40tImage"; Flags: uninsdeletevalue; Tasks: register_40t/atom

Root: HKCR; Subkey: ".pds"; ValueType: string; ValueName: ""; ValueData: "PravetzImage"; Flags: uninsdeletevalue; Tasks: register_pds/pravetz

[Ini]
Filename: {app}\emulator.ini; Section: Environment; Key: Lang; String: russian; Languages: ru
Filename: {app}\emulator.ini; Section: Environment; Key: Lang; String: english; Languages: en
Filename: {app}\emulator.ini; Section: Environment; Key: Flags; String: 526
