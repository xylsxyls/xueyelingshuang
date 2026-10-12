; Build with Inno Setup 6.3+; UTF-8 without BOM.
; sh setup.sh packages the product beside the script, using matching installed SDKs.
; App and Test share this directory; the installer contains the product only.

#if Ver < 0x06030000
  #error Use Inno Setup 6.3 or newer.
#endif
#ifndef AppVersion
  #define AppVersion "1.0"
#endif
#ifndef ProductDir
  #define ProductDir SourcePath
#endif
#ifndef QtDir
  #define QtDir GetEnv("QTDIR")
  #if QtDir == ""
    #define QtDir GetEnv("SystemDrive") + "/Qt/Qt5.5.1_64_2013/5.5/msvc2013_64"
  #endif
#endif
#if !FileExists(QtDir + "\bin\Qt5Core.dll")
  #error Set QTDIR or pass /DQtDir=<Qt 5.5.1 MSVC2013 x64 SDK>.
#endif
#ifndef VC120Dir
  #define VC120Dir GetEnv("VC120DIR")
  #if VC120Dir == ""
    #if GetEnv("VS120COMNTOOLS") != ""
      #define VC120Dir GetEnv("VS120COMNTOOLS") + "/../../VC/redist/x64/Microsoft.VC120.CRT"
    #else
      #define VC120Dir GetEnv("SystemDrive") + "/Program Files (x86)/Microsoft Visual Studio 12.0/VC/redist/x64/Microsoft.VC120.CRT"
    #endif
  #endif
#endif
#if !FileExists(VC120Dir + "\msvcp120.dll") || !FileExists(VC120Dir + "\msvcr120.dll")
  #error Set VC120DIR or pass /DVC120Dir=<VS2013 x64 Microsoft.VC120.CRT directory>.
#endif
#ifndef QtSourceDir
  #define QtSourceDir GetEnv("QTSOURCEDIR")
  #if QtSourceDir == ""
    #define QtSourceDir QtDir + "/../Src"
  #endif
#endif
#if !FileExists(QtSourceDir + "\qtbase\LICENSE.LGPLv21")
  #error Set QTSOURCEDIR or pass /DQtSourceDir=<matching Qt source tree containing license notices>.
#endif
#ifndef InstallerOutputDir
  #define InstallerOutputDir GetEnv("SPLITVIEWER_INSTALLER_OUTPUT_DIR")
  #if InstallerOutputDir == ""
    #define InstallerOutputDir SourcePath
  #endif
#endif
#ifndef CompressionDictionarySize
  #define CompressionDictionarySize 262144
#endif

[Setup]
AppId={{B59158D8-0871-4F27-ACEC-0099F825EE5C}
AppName=分屏看图
AppVersion={#AppVersion}
AppVerName=分屏看图 {#AppVersion}
DefaultDirName={localappdata}\Programs\SplitViewer
DefaultGroupName=分屏看图
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible and not arm64
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=6.1
OutputDir={#InstallerOutputDir}
OutputBaseFilename=SplitViewer-{#AppVersion}-Windows-x64-Setup
Compression=lzma2/ultra64
SolidCompression=yes
LZMADictionarySize={#CompressionDictionarySize}
LZMANumFastBytes=273
LZMAUseSeparateProcess=yes
WizardStyle=modern
UninstallDisplayIcon={app}\SplitViewer1.0.exe
CloseApplications=yes
CloseApplicationsFilter=SplitViewer1.0.exe,*.dll
RestartApplications=no

[Languages]
#if FileExists(CompilerPath + "\Languages\ChineseSimplified.isl")
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
#else
Name: "english"; MessagesFile: "compiler:Default.isl"
#endif

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "快捷方式："; Flags: unchecked

[Files]
; Explicit release inputs exclude Test, symbols, reports, logs and documents.
Source: "{#ProductDir}\SplitViewer1.0.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\SplitViewerCore.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\CStringManager.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\CSystem.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\CTaskThreadManager.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\Semaphore.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\ReadWriteMutex.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\IntDateTime.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\LogManager.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\CDump.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\QssString.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\QtControls.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\DialogManager.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\EO_XmlSax2Parser.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\res\*"; DestDir: "{app}\res"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#ProductDir}\licenses\*"; DestDir: "{app}\licenses"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

; Official Qt libraries and plugins remain outside common and are read from the SDK.
Source: "{#QtDir}\bin\Qt5Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}\bin\Qt5Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}\bin\Qt5Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}\bin\libEGL.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}\bin\libGLESv2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}\bin\opengl32sw.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}\bin\D3Dcompiler_47.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}\plugins\platforms\qwindows.dll"; DestDir: "{app}\plugins\platforms"; Flags: ignoreversion
Source: "{#QtDir}\plugins\imageformats\qgif.dll"; DestDir: "{app}\plugins\imageformats"; Flags: ignoreversion
Source: "{#QtDir}\plugins\imageformats\qico.dll"; DestDir: "{app}\plugins\imageformats"; Flags: ignoreversion
Source: "{#QtDir}\plugins\imageformats\qjpeg.dll"; DestDir: "{app}\plugins\imageformats"; Flags: ignoreversion
Source: "{#QtDir}\plugins\imageformats\qtiff.dll"; DestDir: "{app}\plugins\imageformats"; Flags: ignoreversion
Source: "{#QtDir}\plugins\imageformats\qwebp.dll"; DestDir: "{app}\plugins\imageformats"; Flags: ignoreversion
Source: "{#VC120Dir}\msvcp120.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#VC120Dir}\msvcr120.dll"; DestDir: "{app}"; Flags: ignoreversion

Source: "{#QtSourceDir}\qtbase\LICENSE.LGPLv21"; DestDir: "{app}\licenses\Qt"; Flags: ignoreversion
Source: "{#QtSourceDir}\qtbase\LICENSE.LGPLv3"; DestDir: "{app}\licenses\Qt"; Flags: ignoreversion
Source: "{#QtSourceDir}\qtbase\LGPL_EXCEPTION.txt"; DestDir: "{app}\licenses\Qt"; Flags: ignoreversion
Source: "{#QtSourceDir}\qtbase\src\3rdparty\libjpeg\README"; DestDir: "{app}\licenses\Qt\libjpeg"; Flags: ignoreversion
Source: "{#QtSourceDir}\qtimageformats\src\3rdparty\libtiff\COPYRIGHT"; DestDir: "{app}\licenses\Qt\libtiff"; Flags: ignoreversion
Source: "{#QtSourceDir}\qtimageformats\src\3rdparty\libwebp\COPYING"; DestDir: "{app}\licenses\Qt\libwebp"; Flags: ignoreversion
Source: "{#QtSourceDir}\qtbase\src\3rdparty\angle\LICENSE"; DestDir: "{app}\licenses\Qt\angle"; Flags: ignoreversion

[INI]
Filename: "{app}\qt.conf"; Section: "Paths"; Key: "Plugins"; String: "plugins"; Flags: uninsdeleteentry uninsdeletesectionifempty

[Icons]
Name: "{autoprograms}\分屏看图"; Filename: "{app}\SplitViewer1.0.exe"; WorkingDir: "{app}"; IconFilename: "{app}\SplitViewer1.0.exe"
Name: "{autodesktop}\分屏看图"; Filename: "{app}\SplitViewer1.0.exe"; WorkingDir: "{app}"; IconFilename: "{app}\SplitViewer1.0.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\SplitViewer1.0.exe"; WorkingDir: "{app}"; Description: "启动分屏看图"; Flags: nowait postinstall skipifsilent unchecked