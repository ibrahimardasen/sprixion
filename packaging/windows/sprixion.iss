; Built by deploy.ps1, which passes /DDistDir (the windeployqt output folder).
[Setup]
AppName=Sprixion
AppVersion=0.1.0
DefaultDirName={autopf}\Sprixion
DefaultGroupName=Sprixion
LicenseFile=..\..\LICENSE
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=Sprixion-0.1.0-setup
UninstallDisplayIcon={app}\sprixion.exe
Compression=lzma2
SolidCompression=yes

[Files]
Source: "{#DistDir}\*"; DestDir: "{app}"; Flags: recursesubdirs ignoreversion

[Icons]
Name: "{group}\Sprixion"; Filename: "{app}\sprixion.exe"
