; K-54 · Keller54 · by Frank E. – Windows-Installer (Inno Setup 6)
; Wird von der GitHub-Action gebaut:  ISCC /DAppVersion=1.4.1 /DBuildDir=... K54.iss

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\..\build\K54_artefacts\Release"
#endif

[Setup]
AppId={{6F0A54C2-5405-4E11-9B54-0000004B0054}}
AppName=K-54
AppVersion={#AppVersion}
AppVerName=K-54 {#AppVersion}
AppPublisher=Keller54 · by Frank E.
AppPublisherURL=https://keller54.de
AppCopyright=© Keller54 · Frank E.
VersionInfoCompany=Keller54 · Frank E.
VersionInfoDescription=K-54 Setup (Keller54)
VersionInfoProductName=K-54
VersionInfoCopyright=© Keller54 · Frank E.
AppSupportURL=https://keller54.de/k54/
DefaultDirName={autopf}\Keller54\K-54
DefaultGroupName=Keller54
DisableProgramGroupPage=yes
OutputBaseFilename=K-54-{#AppVersion}-Setup
OutputDir=..\..\dist
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayName=K-54 (Keller54)
CloseApplications=yes
MinVersion=10.0

[Languages]
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Messages]
de.WelcomeLabel2=Dieses Programm installiert den Ladder-Synthesizer K-54 mit Looper, Drum-Computer und Step-Sequencer.%n%nBitte schliesse vorher deine DAW (Ableton, Cubase, Bitwig, FL Studio ...).
en.WelcomeLabel2=This will install the K-54 ladder synthesizer with looper, drum computer and step sequencer.%n%nPlease close your DAW before continuing.

[Types]
Name: "full"; Description: "VST3-Plugin + K-54 App"
Name: "custom"; Description: "Benutzerdefiniert"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3-Plugin (Ableton, Cubase, Bitwig, FL Studio, Komplete Kontrol ...)"; Types: full custom; Flags: fixed
Name: "app";  Description: "K-54 App (ohne DAW spielen)"; Types: full

[Files]
Source: "{#BuildDir}\VST3\K-54.vst3\*"; DestDir: "{commoncf64}\VST3\K-54.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\Standalone\K-54.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion
Source: "Anleitung-Windows.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme

[Icons]
Name: "{group}\K-54"; Filename: "{app}\K-54.exe"; Components: app
Name: "{group}\K-54 Anleitung"; Filename: "{app}\Anleitung-Windows.txt"
Name: "{group}\K-54 deinstallieren"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\Anleitung-Windows.txt"; Description: "Anleitung anzeigen"; Flags: postinstall shellexec skipifsilent unchecked

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\K-54.vst3"
