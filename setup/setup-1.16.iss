; -- Example1.iss --
; Demonstrates copying 3 files and creating an icon.

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!


#define AppVer "1.16"


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
Name: ru; MessagesFile: compiler:Russian.isl
Name: en; MessagesFile: compiler:Default.isl

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


[Ini]
Filename: {app}\emulator.ini; Section: Environment; Key: Lang; String: russian; Languages: ru
Filename: {app}\emulator.ini; Section: Environment; Key: Lang; String: english; Languages: en
