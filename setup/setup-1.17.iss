; -- Example1.iss --
; Demonstrates copying 3 files and creating an icon.

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!


#define AppVer "1.17"


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
Name: ru; MessagesFile: compiler:Russian.isl; InfobeforeFile:"../release-{#AppVer}-ru.txt"
Name: en; MessagesFile: compiler:Default.isl; InfobeforeFile:"../release-{#AppVer}.txt"

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
Name: registerdsk; Description: Регистрация типов образов дисков; Languages: ru
Name: registerdsk; Description: Register disk image extensions; Languages: en
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

Root: HKCR; Subkey: "AppleImage"; ValueType: string; ValueName: ""; ValueData: "Apple ][ Disk Image"; Flags: uninsdeletekey; Tasks: registerdsk; Languages: en
Root: HKCR; Subkey: "AppleImage"; ValueType: string; ValueName: ""; ValueData: "Образ диска Apple ]["; Flags: uninsdeletekey; Tasks: registerdsk; Languages: ru
Root: HKCR; Subkey: "AppleImage\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\emulator.exe,1"; Tasks: registerdsk
Root: HKCR; Subkey: "AppleImage\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(3) Apple ][ Plus"" -cd ""{app}"" -fs 1 -s6d1 ""%1"""; Tasks: registerdsk
Root: HKCR; Subkey: "AppleImage\shell\open_agat7\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(1) IKP-7"" -cd ""{app}"" -fs 1 -s3d1 ""%1"""; Tasks: registerdsk
Root: HKCR; Subkey: "AppleImage\shell\open_agat9\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(2) IKP-9"" -cd ""{app}"" -fs 1 -s5d1 ""%1"""; Tasks: registerdsk
Root: HKCR; Subkey: "AppleImage\shell\open"; ValueType: string; ValueName: ""; ValueData: "Open for Apple ]["; Tasks: registerdsk; Languages: en
Root: HKCR; Subkey: "AppleImage\shell\open"; ValueType: string; ValueName: ""; ValueData: "Открыть для Apple ]["; Tasks: registerdsk; Languages: ru
Root: HKCR; Subkey: "AppleImage\shell\open_agat7"; ValueType: string; ValueName: ""; ValueData: "Open for Agat-7"; Tasks: registerdsk; Languages: en
Root: HKCR; Subkey: "AppleImage\shell\open_agat7"; ValueType: string; ValueName: ""; ValueData: "Открыть для Агат-7"; Tasks: registerdsk; Languages: ru
Root: HKCR; Subkey: "AppleImage\shell\open_agat9"; ValueType: string; ValueName: ""; ValueData: "Open for Agat-9"; Tasks: registerdsk; Languages: en
Root: HKCR; Subkey: "AppleImage\shell\open_agat9"; ValueType: string; ValueName: ""; ValueData: "Открыть для Агат-9"; Tasks: registerdsk; Languages: ru

Root: HKCR; Subkey: "AgatImage"; ValueType: string; ValueName: ""; ValueData: "Agat Disk Image"; Flags: uninsdeletekey; Tasks: registerdsk; Languages: en
Root: HKCR; Subkey: "AgatImage"; ValueType: string; ValueName: ""; ValueData: "Образ диска ПЭВМ Агат"; Flags: uninsdeletekey; Tasks: registerdsk; Languages: ru
Root: HKCR; Subkey: "AgatImage\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\emulator.exe,1"; Tasks: registerdsk
Root: HKCR; Subkey: "AgatImage\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(2) IKP-9"" -cd ""{app}"" -fs 1 -s5d1 ""%1"""; Tasks: registerdsk
Root: HKCR; Subkey: "AgatImage\shell\open_agat7\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(1) IKP-7"" -cd ""{app}"" -fs 1 -s3d1 ""%1"""; Tasks: registerdsk
Root: HKCR; Subkey: "AgatImage\shell\open_apple2\command"; ValueType: string; ValueName: ""; ValueData: """{app}\emulator.exe"" ""(3) Apple ][ Plus"" -cd ""{app}"" -fs 1 -s6d1 ""%1"""; Tasks: registerdsk
Root: HKCR; Subkey: "AgatImage\shell\open"; ValueType: string; ValueName: ""; ValueData: "Open for Agat-9"; Tasks: registerdsk; Languages: en
Root: HKCR; Subkey: "AgatImage\shell\open"; ValueType: string; ValueName: ""; ValueData: "Открыть для Агат-9"; Tasks: registerdsk; Languages: ru
Root: HKCR; Subkey: "AgatImage\shell\open_agat7"; ValueType: string; ValueName: ""; ValueData: "Open for Agat-7"; Tasks: registerdsk; Languages: en
Root: HKCR; Subkey: "AgatImage\shell\open_agat7"; ValueType: string; ValueName: ""; ValueData: "Открыть для Агат-7"; Tasks: registerdsk; Languages: ru
Root: HKCR; Subkey: "AgatImage\shell\open_apple2"; ValueType: string; ValueName: ""; ValueData: "Open for Apple ]["; Tasks: registerdsk; Languages: en
Root: HKCR; Subkey: "AgatImage\shell\open_apple2"; ValueType: string; ValueName: ""; ValueData: "Открыть для Apple ]["; Tasks: registerdsk; Languages: ru

Root: HKCR; Subkey: ".dsk"; ValueType: string; ValueName: ""; ValueData: "AppleImage"; Flags: uninsdeletevalue; Tasks: registerdsk; Languages: en
Root: HKCR; Subkey: ".nib"; ValueType: string; ValueName: ""; ValueData: "AppleImage"; Flags: uninsdeletevalue; Tasks: registerdsk; Languages: en
Root: HKCR; Subkey: ".dsk"; ValueType: string; ValueName: ""; ValueData: "AgatImage"; Flags: uninsdeletevalue; Tasks: registerdsk; Languages: ru
Root: HKCR; Subkey: ".nib"; ValueType: string; ValueName: ""; ValueData: "AgatImage"; Flags: uninsdeletevalue; Tasks: registerdsk; Languages: ru
Root: HKCR; Subkey: ".aim"; ValueType: string; ValueName: ""; ValueData: "AgatImage"; Flags: uninsdeletevalue; Tasks: registerdsk

[Ini]
Filename: {app}\emulator.ini; Section: Environment; Key: Lang; String: russian; Languages: ru
Filename: {app}\emulator.ini; Section: Environment; Key: Lang; String: english; Languages: en
