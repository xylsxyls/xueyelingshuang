@echo off
setlocal
if "%~1"=="" (
    echo Usage: build-windows.cmd absolute-work-directory
    exit /b 2
)
set "WORK=%~f1"
set "ROOT=%~dp0"
if not defined VS120COMNTOOLS (
    echo VS120COMNTOOLS must identify Visual Studio 2013.
    exit /b 2
)
if not exist "%WORK%" mkdir "%WORK%"
if not defined SEVENZIP set "SEVENZIP=%ProgramFiles%\7-Zip\7z.exe"
"%SEVENZIP%" e -y "-o%WORK%" "%ROOT%sonic-0.2.0-luma-r1.tar.xz"
if errorlevel 1 exit /b 1
tar -xf "%WORK%\sonic-0.2.0-luma-r1.tar" -C "%WORK%"
if errorlevel 1 exit /b 1
call :architecture x86 32 x86
if errorlevel 1 exit /b 1
call :architecture amd64 64 x64
exit /b %errorlevel%

:architecture
call "%VS120COMNTOOLS%..\..\VC\vcvarsall.bat" %1
if errorlevel 1 exit /b 1
set "BITS=%2"
set "DEST=%ROOT%lib\%3"
call :variant MT "" ""
if errorlevel 1 exit /b 1
call :variant MTd d ""
if errorlevel 1 exit /b 1
call :variant MD "" MD
if errorlevel 1 exit /b 1
call :variant MDd d MD
exit /b %errorlevel%

:variant
set "OUT=%DEST%\%~3"
set "OBJ=%WORK%\sonic-%BITS%-%1.obj"
set "OPT=/O2 /DNDEBUG"
if "%~2"=="d" set "OPT=/Od /D_DEBUG"
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /c /TP /W3 /GS /Z7 %OPT% /%1 /Fo"%OBJ%" "%WORK%\sonic-0.2.0-luma-r1\sonic.c"
if errorlevel 1 exit /b 1
lib /nologo /OUT:"%OUT%\sonicstatic%BITS%%~2.lib" "%OBJ%"
exit /b %errorlevel%