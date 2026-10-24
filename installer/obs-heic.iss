#define AppName "OBS HEIC Drag & Drop Plugin"
#define AppVersion "1.1.0"
#define AppPublisher "OBS HEIC Plugin Project"
#define ObsExe "{pf64}\obs-studio\bin\64bit\obs64.exe"

[Setup]
AppId={{A0A0102B-C8A7-450C-B3F9-D3F51B917FC0}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
VersionInfoVersion=1.1.0.0
VersionInfoProductVersion={#AppVersion}
VersionInfoProductName={#AppName}
VersionInfoCompany={#AppPublisher}
VersionInfoDescription=HEIC/HEIF drag-and-drop support for OBS Studio
#ifdef QATest
DefaultDirName={localappdata}\OBS-HEIC-Installer-QA
PrivilegesRequired=lowest
#else
DefaultDirName={commonappdata}\obs-studio\plugins\obs-heic
PrivilegesRequired=admin
#endif
DisableDirPage=yes
DisableProgramGroupPage=yes
DirExistsWarning=no
UsePreviousAppDir=no
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763
WizardStyle=modern
Compression=lzma2/ultra64
SolidCompression=yes
OutputDir=output
#ifdef QATest
OutputBaseFilename=OBS-HEIC-Plugin-Setup-{#AppVersion}-QA
#else
OutputBaseFilename=OBS-HEIC-Plugin-Setup-{#AppVersion}
#endif
SetupLogging=yes
CloseApplications=yes
CloseApplicationsFilter=obs64.exe
RestartApplications=no
UninstallDisplayName={#AppName}
UninstallDisplayIcon={#ObsExe}
LicenseFile=..\LICENSE
InfoBeforeFile=..\README.txt

[Languages]
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "payload\bin\64bit\obs-heic.dll"; DestDir: "{app}\bin\64bit"; Flags: ignoreversion
Source: "payload\bin\64bit\heif.dll"; DestDir: "{app}\bin\64bit"; Flags: ignoreversion
Source: "payload\bin\64bit\libde265.dll"; DestDir: "{app}\bin\64bit"; Flags: ignoreversion
Source: "payload\data\locale\en-US.ini"; DestDir: "{app}\data\locale"; Flags: ignoreversion
Source: "payload\data\locale\ja-JP.ini"; DestDir: "{app}\data\locale"; Flags: ignoreversion
Source: "..\README.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\THIRD_PARTY_NOTICES.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "payload\licenses\libheif.txt"; DestDir: "{app}\licenses"; Flags: ignoreversion
Source: "payload\licenses\libde265.txt"; DestDir: "{app}\licenses"; Flags: ignoreversion

[Run]
Filename: "{#ObsExe}"; Description: "OBS Studioを起動する"; Flags: nowait postinstall skipifsilent; Check: FileExists(ExpandConstant('{#ObsExe}'))
Filename: "{sys}\notepad.exe"; Parameters: """{app}\README.txt"""; Description: "READMEを開く"; Flags: nowait postinstall skipifsilent unchecked

[Code]
function InitializeSetup(): Boolean;
var
  ObsPath: String;
begin
  ObsPath := ExpandConstant('{#ObsExe}');
  if not FileExists(ObsPath) then
  begin
    if ActiveLanguage = 'japanese' then
      MsgBox('OBS Studio（64ビット版）が標準の場所に見つかりません。' + #13#10 +
        'OBS Studio 31以降を先にインストールしてください。', mbError, MB_OK)
    else
      MsgBox('64-bit OBS Studio was not found in the standard location.' + #13#10 +
        'Install OBS Studio 31 or later first.', mbError, MB_OK);
    Result := False;
    exit;
  end;
  Result := True;
end;
