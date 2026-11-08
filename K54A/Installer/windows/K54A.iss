; K-54 Analyzer · Keller54 · by Frank E. – Windows-Installer (Inno Setup 6)
; Wird von der GitHub-Action gebaut:  ISCC /DAppVersion=1.0.0 /DBuildDir=... K54A.iss

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\..\build\K54Analyzer_artefacts\Release"
#endif

[Setup]
AppId={{B3C1F5A2-54E5-4D11-9B54-0000004B5A41}}
AppName=K-54 Analyzer
AppVersion={#AppVersion}
AppVerName=K-54 Analyzer {#AppVersion}
AppPublisher=Keller54 · by Frank E.
AppPublisherURL=https://keller54.de
AppSupportURL=https://keller54.de
DefaultDirName={autopf}\Keller54\K-54 Analyzer
DefaultGroupName=Keller54
DisableProgramGroupPage=yes
OutputBaseFilename=K-54-Analyzer-{#AppVersion}-Setup
OutputDir=..\..\dist
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayName=K-54 Analyzer (Keller54)
CloseApplications=yes
MinVersion=10.0

[Languages]
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Messages]
de.WelcomeLabel2=Dieses Programm installiert den K-54 Analyzer (Spektrum, Pegel, LUFS, Referenz-A/B).%n%nBitte schliesse vorher deine DAW (Ableton, Cubase, Bitwig, FL Studio ...).
en.WelcomeLabel2=This will install the K-54 Analyzer (spectrum, levels, LUFS, reference A/B).%n%nPlease close your DAW before continuing.

[Types]
Name: "full"; Description: "VST3-Plugin + K-54 Analyzer App"
Name: "custom"; Description: "Benutzerdefiniert"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3-Plugin (Ableton, Cubase, Bitwig, FL Studio ...)"; Types: full custom; Flags: fixed
Name: "app";  Description: "K-54 Analyzer App (misst den Audio-Eingang ohne DAW)"; Types: full

[Files]
Source: "{#BuildDir}\VST3\K-54 Analyzer.vst3\*"; DestDir: "{commoncf64}\VST3\K-54 Analyzer.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\Standalone\K-54 Analyzer.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion
Source: "Anleitung-Windows.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme

[Icons]
Name: "{group}\K-54 Analyzer"; Filename: "{app}\K-54 Analyzer.exe"; Components: app
Name: "{group}\K-54 Analyzer Anleitung"; Filename: "{app}\Anleitung-Windows.txt"
Name: "{group}\K-54 Analyzer deinstallieren"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\Anleitung-Windows.txt"; Description: "Anleitung anzeigen"; Flags: postinstall shellexec skipifsilent unchecked

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\K-54 Analyzer.vst3"
