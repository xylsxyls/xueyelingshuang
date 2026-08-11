@echo off
setlocal

set CIVET_HOME=%~dp0
set CIVET_SRC=%CIVET_HOME%src
set CIVET_INCLUDE=%CIVET_HOME%include
set CIVET_LIB=%CIVET_HOME%lib
set REPO_ROOT=%CIVET_HOME%..\..
set CIVET_BUILD=%REPO_ROOT%\src\temp\civetweb_build

if not exist "%CIVET_SRC%\civetweb.c" (
    echo civetweb.c not found in "%CIVET_SRC%".
    exit /b 1
)

if "%VS120COMNTOOLS%"=="" (
    set VS120COMNTOOLS=C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\
)

call :build_one x86 Win32 Release X86 MT O2 NDEBUG civetwebstatic32.lib
if errorlevel 1 exit /b 1
call :build_one x86 Win32 Debug X86 MDd Od _DEBUG civetwebstatic32d.lib
if errorlevel 1 exit /b 1
call :build_one x86_amd64 x64 Release X64 MT O2 NDEBUG civetwebstatic64.lib
if errorlevel 1 exit /b 1
call :build_one x86_amd64 x64 Debug X64 MDd Od _DEBUG civetwebstatic64d.lib
if errorlevel 1 exit /b 1

echo CivetWeb VS2013 libraries built successfully.
exit /b 0

:build_one
set VCVARS_ARCH=%~1
set PLATFORM_NAME=%~2
set CONFIG_NAME=%~3
set MACHINE_NAME=%~4
set CRT_NAME=%~5
set OPT_NAME=%~6
set CONFIG_DEFINE=%~7
set OUTPUT_NAME=%~8
set OBJ_DIR=%CIVET_BUILD%\%PLATFORM_NAME%\%CONFIG_NAME%

echo Building %OUTPUT_NAME% with /%CRT_NAME%...
call "%VS120COMNTOOLS%..\..\VC\vcvarsall.bat" %VCVARS_ARCH%
if errorlevel 1 exit /b 1

if not exist "%OBJ_DIR%" mkdir "%OBJ_DIR%"

cl /nologo /c /%CRT_NAME% /%OPT_NAME% /D%CONFIG_DEFINE% /DNO_SSL /D_CRT_SECURE_NO_WARNINGS /I"%CIVET_INCLUDE%" /Fo"%OBJ_DIR%\civetweb.obj" "%CIVET_SRC%\civetweb.c"
if errorlevel 1 exit /b 1

cl /nologo /c /%CRT_NAME% /%OPT_NAME% /EHsc /D%CONFIG_DEFINE% /DNO_SSL /D_CRT_SECURE_NO_WARNINGS /I"%CIVET_INCLUDE%" /Fo"%OBJ_DIR%\CivetServer.obj" "%CIVET_SRC%\CivetServer.cpp"
if errorlevel 1 exit /b 1

lib /NOLOGO /MACHINE:%MACHINE_NAME% /OUT:"%CIVET_LIB%\%OUTPUT_NAME%" "%OBJ_DIR%\civetweb.obj" "%OBJ_DIR%\CivetServer.obj"
if errorlevel 1 exit /b 1

exit /b 0
