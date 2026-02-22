; QTChat Installer Script for Inno Setup
; Version: 1.0.0

#define MyAppName "QTChat"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "QTChat"
#define MyAppURL "https://github.com/qtchat/obs-plugin"
#define MyAppExeName "obs64.exe"

[Setup]
; Basic installer information
AppId={{E8A5B2C1-3D4F-5E6A-7B8C-9D0E1F2A3B4C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

; Installation directories
DefaultDirName={autopf}\obs-studio
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes

; Output settings
OutputDir=..\dist
OutputBaseFilename=QTChat-Setup-{#MyAppVersion}-x64
Compression=lzma2/ultra64
SolidCompression=yes
LZMAUseSeparateProcess=yes

; Appearance
WizardStyle=modern
SetupIconFile=
LicenseFile=LICENSE.txt

; Privileges
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog

; Architecture
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

; Uninstaller
UninstallDisplayIcon={app}\obs-plugins\64bit\QTChat.dll
UninstallDisplayName={#MyAppName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Main plugin DLL
Source: "build\rundir\obs-plugins\64bit\QTChat.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion

; Plugin data (if any)
Source: "build\rundir\data\obs-plugins\QTChat\*"; DestDir: "{app}\data\obs-plugins\QTChat"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}\data\obs-plugins\QTChat"

[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    // Clean up plugin data directory if empty
    DelTree(ExpandConstant('{app}\data\obs-plugins\QTChat'), True, True, True);
  end;
end;
