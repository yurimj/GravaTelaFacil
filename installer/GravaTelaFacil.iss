#define MyAppName "GravaTelaFacil"
#define MyAppVersion "0.1.0"
#define MyAppPublisher "GravaTelaFacil"
#define MyAppExeName "GravaTelaFacil.exe"

[Setup]
AppId={{9F49D6C8-7268-4C8F-A3B1-D481B30189F7}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=..\dist
OutputBaseFilename=GravaTelaFacil-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "..\build\Release\GravaTelaFacil.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\third_party\ffmpeg\ffmpeg.exe"; DestDir: "{app}\tools"; Flags: ignoreversion

[Dirs]
Name: "{userdocs}\..\Videos\GTFacil"

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{group}\Pasta de gravacoes"; Filename: "{userdocs}\..\Videos\GTFacil"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Abrir {#MyAppName}"; Flags: nowait postinstall skipifsilent
