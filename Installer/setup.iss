; ============================================================================
;  ORYMY Bass  -  Instalador (Inno Setup 6)
;  YMY Sounds VST  ·  El sonido de la bachata y el merengue
; ============================================================================
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\build\ORYMYBass_artefacts\Release"
#endif

[Setup]
AppId={{8F3C2A71-5B1D-4E7B-9A55-0C7A4B1D9E22}
AppName=ORYMY Bass
AppVersion={#AppVersion}
AppVerName=ORYMY Bass {#AppVersion}
AppPublisher=YMY Sounds / YMY Records
DefaultDirName={autopf}\YMY Sounds\ORYMY Bass
DefaultGroupName=YMY Sounds
OutputDir=Output
OutputBaseFilename=ORYMY_Bass_Setup_{#AppVersion}
SetupIconFile=icon.ico
UninstallDisplayIcon={app}\ORYMY Bass.exe
WizardStyle=modern
WizardImageFile=wizard_large.bmp
WizardSmallImageFile=wizard_small.bmp
WizardImageStretch=yes
LicenseFile=license_es.txt
Compression=lzma2/ultra64
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
DisableProgramGroupPage=yes

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Types]
Name: "full"; Description: "Instalacion completa (VST3 + aplicacion independiente)"
Name: "vst"; Description: "Solo plugin VST3"
Name: "custom"; Description: "Personalizada"; Flags: iscustom

[Components]
Name: "vst3"; Description: "Plugin VST3  (FL Studio, Ableton, Reaper, Cubase...)"; Types: full vst custom; Flags: fixed
Name: "standalone"; Description: "Aplicacion independiente (Standalone)"; Types: full custom

[Files]
Source: "{#BuildDir}\VST3\ORYMY Bass.vst3\*"; DestDir: "{commoncf64}\VST3\ORYMY Bass.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\Standalone\ORYMY Bass.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{group}\ORYMY Bass (Standalone)"; Filename: "{app}\ORYMY Bass.exe"; Components: standalone
Name: "{group}\Desinstalar ORYMY Bass"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\ORYMY Bass.exe"; Description: "Abrir ORYMY Bass"; Flags: nowait postinstall skipifsilent unchecked; Components: standalone

[Messages]
spanish.WelcomeLabel2=Este asistente instalara [name/ver] en tu computadora.%n%nORYMY Bass es el amplificador de bajo de YMY Sounds para bachata y merengue: pedal, ampli, cabina, EQ y efectos en un solo plugin VST3.%n%nCierra FL Studio antes de continuar.
