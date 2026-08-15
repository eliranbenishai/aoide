#define MyAppName "Tramp"
#ifndef MyAppVersion
  #define MyAppVersion "0.1.0"
#endif
#define MyAppPublisher "Proxima Magnifica"
#define MyAppURL "https://tramp.music"
#define MyAppExeName "tramp.exe"

[Setup]
AppId={{8F3C2E91-6B4A-4D11-9E7A-A1B2C3D4E5F6}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppCopyright=Copyright (C) 2026 Proxima Magnifica
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
PrivilegesRequired=admin
OutputDir=..\..\build\windows\installer
OutputBaseFilename=Tramp-{#MyAppVersion}-windows-x64
SetupIconFile=..\..\windows\runner\resources\app_icon.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
ChangesAssociations=yes
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "..\..\build\windows\x64\runner\Release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\THIRD-PARTY-NOTICES.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Run]
Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Visual C++ runtime…"; Flags: waituntilterminated; Check: NeedsVCRedist
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Registry]
Root: HKLM; Subkey: "Software\Classes\Tramp.Audio"; ValueType: string; ValueName: ""; ValueData: "Tramp audio"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Classes\Tramp.Audio\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"
Root: HKLM; Subkey: "Software\Classes\Tramp.Audio\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""
Root: HKLM; Subkey: "Software\Classes\Tramp.Playlist"; ValueType: string; ValueName: ""; ValueData: "Tramp playlist"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Classes\Tramp.Playlist\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"
Root: HKLM; Subkey: "Software\Classes\Tramp.Playlist\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""
Root: HKLM; Subkey: "Software\Classes\.mp3"; ValueType: string; ValueName: ""; ValueData: "Tramp.Audio"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.m4a"; ValueType: string; ValueName: ""; ValueData: "Tramp.Audio"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.aac"; ValueType: string; ValueName: ""; ValueData: "Tramp.Audio"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.flac"; ValueType: string; ValueName: ""; ValueData: "Tramp.Audio"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.wav"; ValueType: string; ValueName: ""; ValueData: "Tramp.Audio"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.ogg"; ValueType: string; ValueName: ""; ValueData: "Tramp.Audio"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.opus"; ValueType: string; ValueName: ""; ValueData: "Tramp.Audio"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.m3u"; ValueType: string; ValueName: ""; ValueData: "Tramp.Playlist"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.m3u8"; ValueType: string; ValueName: ""; ValueData: "Tramp.Playlist"; Flags: uninsdeletevalue

[Code]
function NeedsVCRedist: Boolean;
begin
  Result :=
    (not FileExists(ExpandConstant('{sys}\vcruntime140.dll'))) or
    (not FileExists(ExpandConstant('{sys}\vcruntime140_1.dll'))) or
    (not FileExists(ExpandConstant('{sys}\msvcp140.dll')));
end;
