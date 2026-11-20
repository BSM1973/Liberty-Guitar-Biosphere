#define ProductVersion GetEnv("GUITAR_BIOSPHERE_VERSION")
#define PluginBundle GetEnv("GUITAR_BIOSPHERE_VST3_SOURCE")
#define InstallerOutput GetEnv("GUITAR_BIOSPHERE_OUTPUT")
#define PackageLabel GetEnv("GUITAR_BIOSPHERE_PACKAGE_LABEL")
#if ProductVersion == ""
  #error Missing GUITAR_BIOSPHERE_VERSION
#endif
#if PluginBundle == ""
  #error Missing GUITAR_BIOSPHERE_VST3_SOURCE
#endif
#if InstallerOutput == ""
  #error Missing GUITAR_BIOSPHERE_OUTPUT
#endif
#if PackageLabel == ""
  #error Missing GUITAR_BIOSPHERE_PACKAGE_LABEL
#endif

[Setup]
AppId=LibertyMusicSoftware.GuitarBiosphere
AppName=Guitar Biosphere
AppVersion={#ProductVersion}
AppPublisher=Liberty Music Software
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={uninstallexe}
OutputDir={#InstallerOutput}
OutputBaseFilename=Guitar-Biosphere-v{#ProductVersion}-Windows-x64-{#PackageLabel}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
RestartApplications=no
SetupLogging=yes
VersionInfoVersion={#ProductVersion}

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#PluginBundle}\*"; DestDir: "{commoncf64}\VST3\Guitar Biosphere.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[InstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Guitar Biosphere.vst3"

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Guitar Biosphere.vst3"
