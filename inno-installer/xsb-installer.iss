; Inno Setup script for xSB-2.

#define XSBName "xSB-2"
; Note: Tick this version number every update.
#define XSBVersion "2.4"
#define XSBPublisher "xSB-2 and OpenStarbound Contributors"
#define XSBGitHubURL "https://github.com/FezzedOne/xSB-2"
; Set this to the location of your xSB-2 source code folder.
#define XSBSourcePath "D:\xSB-2"

[Setup]
; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{C3DDE786-7102-45A8-A988-4C30475DC79E}
AppName={#XSBName}
AppVersion={#XSBVersion}
;AppVerName={#XSBName} {#XSBVersion}
AppPublisher={#XSBPublisher}
AppPublisherURL={#XSBGitHubURL}
AppSupportURL={#XSBGitHubURL}
AppUpdatesURL={#XSBGitHubURL}
DefaultDirName=C:\Program Files (x86)\Steam\steamapps\common\Starbound
DefaultGroupName={#XSBName}
; Disable this warning since xSB-2 must be installed in an existing Starbound install.
DirExistsWarning=no
AllowNoIcons=yes
;InfoBeforeFile=D:\xSB-2\inno-installer\install-info.txt
; Uncomment the following line to run in non administrative install mode (install for current user only.)
;PrivilegesRequired=lowest
OutputDir={#XSBSourcePath}\inno-installer\compiled
OutputBaseFilename=windows-install
SetupIconFile={#XSBSourcePath}\source\client\xclient-largelogo.ico
Compression=lzma
SolidCompression=yes
WizardStyle=modern
SourceDir=D:\xSB-2\inno-installer\

[Languages]
Name: "english"; MessagesFile: "English.isl"
Name: "russian"; MessagesFile: "Russian.isl"

[Files]
Source: "{#XSBSourcePath}\inno-installer\xsb-win64\*"; DestDir: "{app}\xsb-win64"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#XSBSourcePath}\inno-installer\xsb-assets\*"; DestDir: "{app}\xsb-assets"; Flags: ignoreversion recursesubdirs createallsubdirs
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\xClient"; FileName: "{app}\xsb-win64\xclient.exe"
Name: "{group}\xSB-2 GitHub"; Filename: "{#XSBGitHubURL}"
Name: "{group}\Uninstall xSB-2"; Filename: "{uninstallexe}"

[Code]
function NextButtonClick(PageId: Integer): Boolean;
begin
    Result := True;
    if (PageId = wpSelectDir) and not FileExists(ExpandConstant('{app}\win64\starbound.exe')) then begin
        MsgBox(ExpandConstant('{cm:StarboundCheckError}'), mbError, MB_OK);
        Result := False;
        exit;
    end;
end;