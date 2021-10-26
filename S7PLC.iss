; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "S7PLC X2 Driver"
#define MyAppVersion "1.1"
#define MyAppPublisher "RTI-Zone"
#define MyAppURL "https://rti-zone.org"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{D3A23E5B-529D-4907-B2E3-310BE18BF67E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={code:TSXInstallDir}\Resources\Common
DefaultGroupName={#MyAppName}

; Need to customise these
; First is where you want the installer to end up
OutputDir=installer
; Next is the name of the installer
OutputBaseFilename=S7PLC_X2_Installer
; Final one is the icon you would like on the installer. Comment out if not needed.
SetupIconFile=rti_zone_logo.ico
Compression=lzma
SolidCompression=yes
; We don't want users to be able to select the drectory since read by TSXInstallDir below
DisableDirPage=yes
; Uncomment this if you don't want an uninstaller.
;Uninstallable=no
CloseApplications=yes
DirExistsWarning=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Dirs]
Name: "{app}\Plugins\DomePlugIns";
Name: "{app}\Plugins64\DomePlugIns";

[Files]
; WIll also need to customise these!
Source: "domelist S7PLC.txt";                       DestDir: "{app}\Miscellaneous Files"; Flags: ignoreversion
Source: "domelist S7PLC.txt";                       DestDir: "{app}\Miscellaneous Files"; DestName: "domelist64 S7PLC.txt"; Flags: ignoreversion
; 32 bits
Source: "libS7PLC\Win32\Release\libS7PLC.dll";      DestDir: "{app}\Plugins\DomePlugIns"; Flags: ignoreversion
Source: "S7PLC.ui";                                 DestDir: "{app}\Plugins\DomePlugIns"; Flags: ignoreversion
Source: "win_libs/Win32/libcurl.dll";               DestDir: "{app}\..\..\"; Flags: ignoreversion

; 64 bits
Source: "libS7PLC\x64\Release\libS7PLC.dll";        DestDir: "{app}\Plugins64\DomePlugIns"; Flags: ignoreversion; Check: DirExists(ExpandConstant('{app}\Plugins64\DomePlugIns'))
Source: "S7PLC.ui";                                 DestDir: "{app}\Plugins64\DomePlugIns"; Flags: ignoreversion; Check: DirExists(ExpandConstant('{app}\Plugins64\DomePlugIns'))
Source: "win_libs/x64/libcurl.dll";                 DestDir: "{app}\..\..\TheSky64"; Flags: ignoreversion; Check: DirExists(ExpandConstant('{app}\..\..\TheSky64'))

; NOTE: Don't use "Flags: ignoreversion" on any shared system files
; msgBox('Do you want to install MyProg.exe to ' + ExtractFilePath(CurrentFileName) + '?', mbConfirmation, MB_YESNO)


[Code]
{* Below is a function to read TheSkyXInstallPath.txt and confirm that the directory does exist
   This is then used in the DefaultDirName above
   *}
var
  Location: String;
  LoadResult: Boolean;

function TSXInstallDir(Param: String) : String;
begin
  LoadResult := LoadStringFromFile(ExpandConstant('{userdocs}') + '\Software Bisque\TheSkyX Professional Edition\TheSkyXInstallPath.txt', Location);
  if not LoadResult then
    LoadResult := LoadStringFromFile(ExpandConstant('{userdocs}') + '\Software Bisque\TheSky Professional Edition 64\TheSkyXInstallPath.txt', Location);
    if not LoadResult then
      RaiseException('Unable to find the installation path for The Sky X');
  if not DirExists(Location) then
    RaiseException('The SkyX installation directory ' + Location + ' does not exist');
  Result := Location;
end;
