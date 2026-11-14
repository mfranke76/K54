; K-54 D · Keller54 · by Frank E. – Windows-Installer (Inno Setup 6)
; Wird von der GitHub-Action gebaut:  ISCC /DAppVersion=1.0.0 /DBuildDir=... K54D.iss

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\..\build\K54D_artefacts\Release"
#endif

[Setup]
AppId={{B3C1F5A2-54E5-4D11-9B54-0000004B5D31}}
AppName=K-54 D
AppVersion={#AppVersion}
AppVerName=K-54 D {#AppVersion}
AppPublisher=Keller54 · by Frank E.
AppPublisherURL=https://keller54.de
AppCopyright=© Keller54 · Frank E.
VersionInfoCompany=Keller54 · Frank E.
VersionInfoDescription=K-54 D Setup (Keller54)
VersionInfoProductName=K-54 D
VersionInfoCopyright=© Keller54 · Frank E.
AppSupportURL=https://keller54.de
DefaultDirName={autopf}\Keller54\K-54 D
DefaultGroupName=Keller54
DisableProgramGroupPage=yes
OutputBaseFilename=K-54-D-{#AppVersion}-Setup
OutputDir=..\..\dist
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayName=K-54 D (Keller54)
CloseApplications=yes
MinVersion=10.0

[Languages]
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Messages]
de.WelcomeLabel2=Dieses Programm installiert den Drumcomputer K-54 D (808 bis heute, 12 Spuren, 8-32 Steps).%n%nBitte schliesse vorher deine DAW (Ableton, Cubase, Bitwig, FL Studio ...).
en.WelcomeLabel2=This will install the K-54 D drum machine (808 to today, 12 tracks, 8-32 steps).%n%nPlease close your DAW before continuing.

[Types]
Name: "full"; Description: "VST3-Plugin + K-54 D App"
Name: "custom"; Description: "Benutzerdefiniert"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3-Plugin (Ableton, Cubase, Bitwig, FL Studio, Komplete Kontrol ...)"; Types: full custom; Flags: fixed
Name: "app";  Description: "K-54 D App (ohne DAW spielen)"; Types: full

[Files]
Source: "{#BuildDir}\VST3\K-54 D.vst3\*"; DestDir: "{commoncf64}\VST3\K-54 D.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\Standalone\K-54 D.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion
Source: "Anleitung-Windows.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme

[Icons]
Name: "{group}\K-54 D"; Filename: "{app}\K-54 D.exe"; Components: app
Name: "{group}\K-54 D Anleitung"; Filename: "{app}\Anleitung-Windows.txt"
Name: "{group}\K-54 D deinstallieren"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\Anleitung-Windows.txt"; Description: "Anleitung anzeigen"; Flags: postinstall shellexec skipifsilent unchecked

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\K-54 D.vst3"
