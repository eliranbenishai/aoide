#define MyAppName "Aoide"
#ifndef MyAppVersion
  #define MyAppVersion "1.0"
#endif
#define MyAppPublisher "Proxima Magnifica"
#define MyAppURL "https://aoide.music"
#define MyAppExeName "aoide.exe"

[Setup]
AppId={{1539FF2A-1195-4048-8168-9B6DE967BA75}
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
OutputBaseFilename=Aoide-{#MyAppVersion}-windows-x64
SetupIconFile=app_icon.ico
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
Source: "..\..\build\windows\stage\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
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
Root: HKLM; Subkey: "Software\Classes\Aoide.Audio"; ValueType: string; ValueName: ""; ValueData: "Aoide audio"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Classes\Aoide.Audio\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"
Root: HKLM; Subkey: "Software\Classes\Aoide.Audio\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""
Root: HKLM; Subkey: "Software\Classes\Aoide.Playlist"; ValueType: string; ValueName: ""; ValueData: "Aoide playlist"; Flags: uninsdeletekey
Root: HKLM; Subkey: "Software\Classes\Aoide.Playlist\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"
Root: HKLM; Subkey: "Software\Classes\Aoide.Playlist\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""
Root: HKLM; Subkey: "Software\Classes\.mp3"; ValueType: string; ValueName: ""; ValueData: "Aoide.Audio"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.m4a"; ValueType: string; ValueName: ""; ValueData: "Aoide.Audio"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.aac"; ValueType: string; ValueName: ""; ValueData: "Aoide.Audio"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.flac"; ValueType: string; ValueName: ""; ValueData: "Aoide.Audio"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.wav"; ValueType: string; ValueName: ""; ValueData: "Aoide.Audio"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.ogg"; ValueType: string; ValueName: ""; ValueData: "Aoide.Audio"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.opus"; ValueType: string; ValueName: ""; ValueData: "Aoide.Audio"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.m3u"; ValueType: string; ValueName: ""; ValueData: "Aoide.Playlist"; Flags: uninsdeletevalue
Root: HKLM; Subkey: "Software\Classes\.m3u8"; ValueType: string; ValueName: ""; ValueData: "Aoide.Playlist"; Flags: uninsdeletevalue

[Code]
function NeedsVCRedist: Boolean;
begin
  Result :=
    (not FileExists(ExpandConstant('{sys}\vcruntime140.dll'))) or
    (not FileExists(ExpandConstant('{sys}\vcruntime140_1.dll'))) or
    (not FileExists(ExpandConstant('{sys}\msvcp140.dll')));
end;
