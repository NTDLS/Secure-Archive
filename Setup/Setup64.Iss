;#define Debug
#define CompanyName         "NetworkDLS"
#define ApplicationName     "Secure Archive"
#define ApplicationVersion  "1.1.0.0"

[Setup]
;-- Main Setup Information
 AppName                         = {#ApplicationName}
 AppVerName                      = {#ApplicationName} {#ApplicationVersion}
 AppCopyright                    = Copyright © 1995-2025 {#CompanyName}.
 DefaultDirName                  = {commonpf}\{#CompanyName}\{#ApplicationName}
 DefaultGroupName                = {#CompanyName}\{#ApplicationName}
 UninstallDisplayIcon            = {app}\SA.Exe
 PrivilegesRequired              = admin
 Uninstallable                   = Yes
 AppMutex                        = {#ApplicationName}
 SetupIconFile                   = Resources/MainIcon.ico
 LicenseFile                     = Resources/EULA.txt
 Compression                     = ZIP/9
 ChangesAssociations             = Yes
 MinVersion                      = 0.0,6.1sp1
 VersionInfoVersion              = {#ApplicationVersion}
 ArchitecturesInstallIn64BitMode = x64os
 ArchitecturesAllowed            = x64os
 UsedUserAreasWarning            = false
 OutputBaseFilename              = {#ApplicationName}_{#ApplicationVersion}_x64

;-- Windows 2000 & XP (Support Dialog)
 AppPublisher    = {#CompanyName}
 AppPublisherURL = http://www.{#CompanyName}.com/
 AppUpdatesURL   = http://www.{#CompanyName}.com/
 AppVersion      = {#ApplicationVersion}

[Types]
 Name: "Full";    Description: "Full installation"
 Name: "Compact"; Description: "Compact installation"
 Name: "Custom";  Description: "Custom installation"; Flags: iscustom

[Components]
 Name: "App";   Description: "Application";    Types: Full Compact Custom; Flags: fixed;
 Name: "Shell"; Description: "Dependencies";   Types: Full Compact Custom; Flags: fixed;
 Name: "Help";  Description: "Documentation";  Types: Full Custom;
 
[Tasks]
 Name: desktopicon;        Description: "Create a &desktop icon";    GroupDescription: "Additional icons:"; Components: App
 Name: desktopicon\common; Description: "For all users";             GroupDescription: "Additional icons:"; Components: App; Flags: exclusive
 Name: desktopicon\user;   Description: "For the current user only"; GroupDescription: "Additional icons:"; Components: App; Flags: exclusive unchecked

[Files]
 Source: "..\SelfExtractor\Release\x64\SASE.exe";        DestDir: "{app}"; Flags: IgnoreVersion;  Components: App;
 Source: "..\Interface\Release\x64\SA.exe";              DestDir: "{app}"; Flags: IgnoreVersion;  Components: App;
 Source: "..\ShellExtension\Release\x64\ShellExt.Dll";   DestDir: "{app}"; Flags: RestartReplace; Components: Shell;
 Source: "..\HelpFile\Help.chm";                         DestDir: "{app}"; Flags: IgnoreVersion;  Components: Help;

[Icons]
 Name: "{group}\{#ApplicationName}"; Filename: "{app}\SA.Exe";         Components: App
 Name: "{group}\HardKey Generator";  Filename: "{app}\SA.Exe";         Parameters:"/HardKeyGen";   IconFilename:"{app}\SA.Exe"; IconIndex:"5";  Components: App
 Name: "{group}\Secure Delete";      Filename: "{app}\SA.Exe";         Parameters:"/SecureDelete"; IconFilename:"{app}\SA.Exe"; IconIndex:"11"; Components: App
 Name: "{group}\Configure";          Filename: "{app}\SA.Exe";         Parameters:"/Configure";    IconFilename:"{app}\SA.Exe"; IconIndex:"1";  Components: App
 Name: "{group}\Documentation";      Filename: "{app}\Help.chm";       Components: Help
 Name: "{group}\Uninstall";          Filename: "{uninstallexe}";       Components: App

 Name: "{commondesktop}\{#ApplicationName}"; Filename: "{app}\SA.Exe"; Components: App; Tasks: desktopicon\common;
 Name: "{userdesktop}\{#ApplicationName}";   Filename: "{app}\SA.Exe"; Components: App; Tasks: desktopicon\user;

[Run]
 Filename: "{app}\SA.Exe"; Description: "Launch application"; Flags: postinstall nowait skipifsilent runasoriginaluser

[Registry]
;-- Create file type association
 Root: HKCR; Subkey: ".SAkf"; Flags: uninsdeletekey;
 Root: HKCR; Subkey: ".SAkf"; ValueType: string; ValueData: "{#ApplicationName} Hard Key File"

 Root: HKCR; Subkey: "{#ApplicationName} Hard Key File"; Flags: uninsdeletekey;
 Root: HKCR; Subkey: "{#ApplicationName} Hard Key File"; ValueType: string;  ValueData:"SA-Hard Key File"

 Root: HKCR; Subkey: "{#ApplicationName} Hard Key File\Shell\Open\Command"; Flags: uninsdeletekey;
 Root: HKCR; Subkey: "{#ApplicationName} Hard Key File\Shell\Open\Command"; ValueType: string; ValueData:"""{app}\SA.Exe"" /KeyGenFile"

 Root: HKCR; Subkey: "{#ApplicationName} Hard Key File\DefaultIcon"; Flags: uninsdeletekey;
 Root: HKCR; Subkey: "{#ApplicationName} Hard Key File\DefaultIcon"; ValueType: string;  ValueData:"{app}\SA.Exe,5"

 ;--------------------------
 Root: HKCR; Subkey: ".SAef"; Flags: uninsdeletekey;
 Root: HKCR; Subkey: ".SAef"; ValueType: string;  ValueData: "{#ApplicationName} Encrypted File"

 Root: HKCR; Subkey: "{#ApplicationName} Encrypted File"; Flags: uninsdeletekey;
 Root: HKCR; Subkey: "{#ApplicationName} Encrypted File"; ValueType: string;  ValueData:"SA-{#ApplicationName}"

 Root: HKCR; Subkey: "{#ApplicationName} Encrypted File\Shell\Open\Command"; Flags: uninsdeletekey;
 Root: HKCR; Subkey: "{#ApplicationName} Encrypted File\Shell\Open\Command"; ValueType: string;  ValueData:"""{app}\SA.Exe"" /Source:""%1"" /View"

 Root: HKCR; Subkey: "{#ApplicationName} Encrypted File\DefaultIcon"; Flags: uninsdeletekey;
 Root: HKCR; Subkey: "{#ApplicationName} Encrypted File\DefaultIcon"; ValueType: string;  ValueData:"{app}\SA.Exe,0"

;-- Create shell extension values
 Root: HKCR; Subkey: "CLSID\{{8CC98F55-96FA-4bde-B01B-687EDFA6BD36}";                ValueType: string;  ValueData: "{#ApplicationName} Shell Extension"; Flags: uninsdeletekey; Components: Shell;
 Root: HKCR; Subkey: "CLSID\{{8CC98F55-96FA-4bde-B01B-687EDFA6BD36}\InProcServer32"; ValueType: string;  ValueData: "{app}\ShellExt.Dll"; Components: Shell;
 Root: HKCR; Subkey: "CLSID\{{8CC98F55-96FA-4bde-B01B-687EDFA6BD36}\InProcServer32"; ValueType: string;  ValueName: "ThreadingModel"; ValueData:"Apartment"; Components: Shell;

 Root: HKCR; Subkey: "*\shellex\ContextMenuHandlers\{#ApplicationName}";                 ValueType: string;  ValueData:"{{8CC98F55-96FA-4bde-B01B-687EDFA6BD36}"; Flags: uninsdeletekey; Components: Shell;
 Root: HKCR; Subkey: "directory\Shellex\ContextMenuHandlers\{#ApplicationName}";         ValueType: string;  ValueData:"{{8CC98F55-96FA-4bde-B01B-687EDFA6BD36}"; Flags: uninsdeletekey; Components: Shell;
 Root: HKCR; Subkey: "Drive\Shellex\ContextMenuHandlers\{#ApplicationName}";             ValueType: string;  ValueData:"{{8CC98F55-96FA-4bde-B01B-687EDFA6BD36}"; Flags: uninsdeletekey; Components: Shell;

 Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved"; ValueType: string;  ValueName:"{{8CC98F55-96FA-4bde-B01B-687EDFA6BD36}"; ValueData:"{#ApplicationName} Shell Extension"; Flags: deletevalue; Components: Shell;

;------------------------------------------------------------------------------------------------------------------------

 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}"; Flags: uninsdeletekey;

 ;-- Create Path Keys For Use Within {#ApplicationName}
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}\Path";
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}\Path"; ValueType: string;  ValueName: "Path";                      ValueData:"{app}";                Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}\Path"; ValueType: string;  ValueName: "{#ApplicationName}";            ValueData:"{app}\SA.Exe";         Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}\Path"; ValueType: string;  ValueName: "MenuIcon";                  ValueData:"{app}\MenuIcon.Bmp";   Flags: CreateValueIfDoesntExist;

 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "SecureDeletePasses";        ValueData:"0";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "WarnAboutNoEncryption";     ValueData:"1";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "WeakEncryptionWarnLevel";   ValueData:"3";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "AlwaysRunInBackground";     ValueData:"0";                    Flags: CreateValueIfDoesntExist;

 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "UseProcessBoost";           ValueData:"1";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "UseThreadBoost";            ValueData:"1";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "CPUAffinityMask";           ValueData:"0";                    Flags: CreateValueIfDoesntExist;

 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "DfltChkTimeStamps";         ValueData:"1";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "DfltChkIncludeHiddenFiles"; ValueData:"1";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "DfltChkAttributes";         ValueData:"1";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "DfltChkCRCs";               ValueData:"1";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "DfltChkSavePaths";          ValueData:"1";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: String;  ValueName: "DfltEncryptAlgo";           ValueData:"NASCCL";               Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: String;  ValueName: "DfltCompressLevel";         ValueData:"Very High";            Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: String;  ValueName: "DfltTempFilePath";          ValueData:"";                     Flags: CreateValueIfDoesntExist;

 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "DfltShowGridLines";         ValueData:"1";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "DfltShowGridIcons";         ValueData:"1";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "DfltEnableFullRowSelect";   ValueData:"1";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "DfltEnableDragDrop";        ValueData:"1";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "DfltAlwaysOnTop";           ValueData:"0";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "DfltShowGridLines";         ValueData:"1";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "DfltDeleteSourceFiles";     ValueData:"0";                    Flags: CreateValueIfDoesntExist;

 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "DfltDefaultSortColumn";     ValueData:"0";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "DfltDefaultSortDirection";  ValueData:"1";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "KeepSelectionVisible";      ValueData:"1";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "UsePreAllocation";          ValueData:"0";                    Flags: CreateValueIfDoesntExist;
 Root: HKLM; Subkey: "Software\{#CompanyName}\{#ApplicationName}";      ValueType: DWORD;   ValueName: "IgnoreFreeSpaceWarning";    ValueData:"0";

