; ====================================================================
; SABIT - Scientific Astronomical Imaging & Observation Toolkit
; Inno Setup 7 Installer Script
; Author: Mauludy Zahron (Master's Thesis in Ilmu Falak)
; ====================================================================

#define MyAppName "SABIT"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Mauludy Zahron"
#define MyAppURL "https://github.com/MauludyZhrn/Sabit"
#define MyAppExeName "Sabit.exe"
#define MyAppAssocName "SABIT Session Data"
#define MyAppAssocExt ".sabit"
#define MyAppAssocKey StringChange(MyAppAssocName, " ", "") + MyAppAssocExt

[Setup]
; Unique, stable GUID across updates
AppId={{c0324e0d-2a10-44a5-a2de-67b6c293aae4}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

; Per-user installation (no admin elevation required)
DefaultDirName={localappdata}\Programs\Sabit
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest

; Architecture: 64-bit Windows
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

; Distribution output
OutputDir=..\dist
OutputBaseFilename=Sabit-Setup-v1.0.0-x64
SetupIconFile=..\assets\logo.ico
UninstallDisplayIcon={app}\assets\logo.ico

; Compression settings
Compression=lzma2/ultra64
SolidCompression=yes
InternalCompressLevel=ultra64

; Wizard visual appearance
WizardStyle=modern

; Info pages
InfoBeforeFile=..\docs\testing\RELEASE_NOTES_TEST_BUILD.md

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Main application binaries and assets from staging package
Source: "..\dist\package\Sabit\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; Testing and documentation package
Source: "..\docs\testing\*"; DestDir: "{app}\docs\testing"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\assets\logo.ico"
Name: "{group}\Panduan Pengguna (User Manual)"; Filename: "{app}\docs\User_Manual.pdf"; WorkingDir: "{app}"
Name: "{group}\Panduan Evaluasi Riset"; Filename: "{app}\docs\testing\TESTING_GUIDE_ID.md"; WorkingDir: "{app}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\assets\logo.ico"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Clean up temporary runtime logs on uninstall, but preserve user captures/datasets
Type: files; Name: "{app}\app.log"
Type: files; Name: "{app}\qt.conf"
