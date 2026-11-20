; Guitar Biosphere — Windows x64 VST3 installer (Inno Setup 6).
#ifndef MyAppVersion
#define MyAppVersion "0.5.0"
#endif
#ifndef MySourceDir
#define MySourceDir "build\GuitarBiosphere_artefacts\Release\VST3"
#endif
#ifndef MyOutputDir
#define MyOutputDir "release-output"
#endif
#ifndef MyFileSuffix
#define MyFileSuffix "UNSIGNED-TEST"
#endif

[Setup]
AppId={{A9BA8B92-5740-4A93-9EA3-107F283BE105}
AppName=Guitar Biosphere
AppVersion={#MyAppVersion}
AppVerName=Guitar Biosphere {#MyAppVersion}
AppPublisher=Liberty Music Software
AppPublisherURL=https://libertymusicsoftware.com
VersionInfoDescription=Guitar Biosphere VST3 installer
VersionInfoProductName=Guitar Biosphere
VersionInfoProductVersion={#MyAppVersion}
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
OutputDir={#MyOutputDir}
OutputBaseFilename=Guitar-Biosphere-v{#MyAppVersion}-Windows-x64-{#MyFileSuffix}-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
Uninstallable=yes
CreateUninstallRegKey=yes
CloseApplications=yes
RestartApplications=no
UsePreviousAppDir=no

[Languages]
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#MySourceDir}\Guitar Biosphere.vst3\*"; DestDir: "{commoncf64}\VST3\Guitar Biosphere.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: dirifempty; Name: "{commoncf64}\VST3\Guitar Biosphere.vst3"
