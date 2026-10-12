; Build with Inno Setup 6.3+; keep this file UTF-8 without BOM.
; Product paths are relative to this script; Qt/CRT are read from installed SDKs.
; Shell entry: sh ./setup.sh. The default output is beside this script.
; This packages the player only. App/Test deployments still share common/LumaPlayer.

#if Ver < 0x06030000
  #error Use Inno Setup 6.3 or newer for UTF-8 and x64 architecture identifiers.
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
#ifndef VC14Dir
  #define VC14Dir GetEnv("VC14DIR")
  #if VC14Dir == ""
    #if FileExists(ProductDir + "\..\tools\平台库\v14\x64\msvcp140.dll")
      #define VC14Dir ProductDir + "\..\tools\平台库\v14\x64"
    #elif FileExists(ProductDir + "\..\..\tools\平台库\v14\x64\msvcp140.dll")
      #define VC14Dir ProductDir + "\..\..\tools\平台库\v14\x64"
    #else
      #define VC14Dir ProductDir + "\..\..\..\tools\平台库\v14\x64"
    #endif
  #endif
#endif
#if !FileExists(VC14Dir + "\msvcp140.dll") || !FileExists(VC14Dir + "\msvcp140_1.dll") || !FileExists(VC14Dir + "\vcruntime140.dll") || !FileExists(VC14Dir + "\vcruntime140_1.dll")
  #error Supply four matching x64 v14 DLLs in tools/平台库/v14/x64 or set VC14DIR.
#endif
#ifndef InstallerOutputDir
  #define InstallerOutputDir SourcePath
#endif
#ifndef CompressionDictionarySize
  #define CompressionDictionarySize 262144
#endif

[Setup]
AppId={{8A56214F-1D94-4F62-9DCC-C809212C2D6A}
AppName=流光播放器
AppVersion={#AppVersion}
AppVerName=流光播放器 {#AppVersion}
DefaultDirName={localappdata}\Programs\LumaPlayer
DefaultGroupName=流光播放器
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible and not arm64
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir={#InstallerOutputDir}
OutputBaseFilename=LumaPlayer-{#AppVersion}-Windows-x64-Setup
Compression=lzma2/ultra64
SolidCompression=yes
LZMADictionarySize={#CompressionDictionarySize}
LZMANumFastBytes=273
LZMAUseSeparateProcess=yes
WizardStyle=modern
UninstallDisplayIcon={app}\LumaPlayer1.0.exe
CloseApplications=yes
CloseApplicationsFilter=LumaPlayer1.0.exe,*.dll
RestartApplications=no
InfoAfterFile={#ProductDir}\karaoke\licenses\第三方使用说明.txt

[Languages]
#if FileExists(CompilerPath + "\Languages\ChineseSimplified.isl")
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
#else
Name: "english"; MessagesFile: "compiler:Default.isl"
#endif

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "快捷方式："; Flags: unchecked

[Files]
; Explicit runtime list: no Test EXE, symbols, reports, logs or development documents.
Source: "{#VC14Dir}\msvcp140.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#VC14Dir}\msvcp140_1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#VC14Dir}\vcruntime140.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#VC14Dir}\vcruntime140_1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\LumaPlayer1.0.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\LumaPlayerCore.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\KissFFTCpp.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\FFmpegCpp.dll"; DestDir: "{app}"; Flags: ignoreversion
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
Source: "{#ProductDir}\SoundTouchDLL.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\avcodec-60.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\avformat-60.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\avutil-58.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\swresample-4.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\swscale-7.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProductDir}\res\*"; DestDir: "{app}\res"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#ProductDir}\karaoke\onnxruntime.dll"; DestDir: "{app}\karaoke"; Flags: ignoreversion
Source: "{#ProductDir}\karaoke\onnxruntime_providers_shared.dll"; DestDir: "{app}\karaoke"; Flags: ignoreversion
Source: "{#ProductDir}\karaoke\DirectML.dll"; DestDir: "{app}\karaoke"; Flags: ignoreversion
Source: "{#ProductDir}\karaoke\licenses\*"; DestDir: "{app}\karaoke\licenses"; Flags: ignoreversion recursesubdirs createallsubdirs

Source: "{#QtDir}\bin\Qt5Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}\bin\Qt5Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}\bin\Qt5Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}\bin\Qt5Multimedia.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}\bin\Qt5Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}\bin\Qt5Xml.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}\bin\libEGL.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}\bin\libGLESv2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}\bin\opengl32sw.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#QtDir}\plugins\platforms\qwindows.dll"; DestDir: "{app}\plugins\platforms"; Flags: ignoreversion
Source: "{#QtDir}\plugins\audio\qtaudio_windows.dll"; DestDir: "{app}\plugins\audio"; Flags: ignoreversion
Source: "{#QtDir}\plugins\imageformats\qico.dll"; DestDir: "{app}\plugins\imageformats"; Flags: ignoreversion
Source: "{#QtDir}\plugins\imageformats\qjpeg.dll"; DestDir: "{app}\plugins\imageformats"; Flags: ignoreversion
Source: "{#VC120Dir}\msvcp120.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#VC120Dir}\msvcr120.dll"; DestDir: "{app}"; Flags: ignoreversion
; Keep the large model last so related binaries share the solid dictionary.
Source: "{#ProductDir}\karaoke\UVR-MDX-NET-Inst_HQ_5.onnx"; DestDir: "{app}\karaoke"; Flags: ignoreversion

[INI]
Filename: "{app}\qt.conf"; Section: "Paths"; Key: "Plugins"; String: "plugins"; Flags: uninsdeleteentry uninsdeletesectionifempty

[Icons]
Name: "{autoprograms}\流光播放器"; Filename: "{app}\LumaPlayer1.0.exe"; WorkingDir: "{app}"; IconFilename: "{app}\LumaPlayer1.0.exe"
Name: "{autodesktop}\流光播放器"; Filename: "{app}\LumaPlayer1.0.exe"; WorkingDir: "{app}"; IconFilename: "{app}\LumaPlayer1.0.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\LumaPlayer1.0.exe"; WorkingDir: "{app}"; Description: "启动流光播放器"; Flags: nowait postinstall skipifsilent unchecked