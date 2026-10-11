@echo off
setlocal
set "DEST=%~dp0"
set "WORK=%DEST%build"
set "TEMP=%WORK%\temp"
set "TMP=%TEMP%"
if not exist "%TEMP%" mkdir "%TEMP%"
pushd "%DEST%"
tar -xf soundtouch-2.4.0.tar.gz -C build --exclude=libSoundTouchDll.so
if errorlevel 1 exit /b 1
popd
set "SOURCE=%WORK%\soundtouch"
for %%A in (x86 x64) do (
    call :arch %%A
    if errorlevel 1 exit /b 1
)
if not exist "%DEST%include\soundtouchwindows" mkdir "%DEST%include\soundtouchwindows"
copy /y "%SOURCE%\include\*.h" "%DEST%include\soundtouchwindows\" >nul
if errorlevel 1 exit /b 1
copy /y "%SOURCE%\source\SoundTouchDLL\SoundTouchDLL.h" "%DEST%include\soundtouchwindows\" >nul
if errorlevel 1 exit /b 1
exit /b 0
:arch
setlocal
set "ARCH=%1"
set "BITS=32"
set "VCARCH=x86"
if "%ARCH%"=="x64" set "BITS=64"
if "%ARCH%"=="x64" set "VCARCH=x86_amd64"
call "%VS120COMNTOOLS%..\..\VC\vcvarsall.bat" %VCARCH%
if errorlevel 1 exit /b 1
for %%R in (MT MD MTd MDd) do (
    call :crt %%R
    if errorlevel 1 exit /b 1
)
exit /b 0
:crt
set "OBJ=%WORK%\%ARCH%\%1"
set "LIBDIR=%DEST%lib\%ARCH%"
set "SUFFIX="
set "OPT=/O2"
if "%1"=="MD" set "LIBDIR=%LIBDIR%\MD"
if "%1"=="MDd" set "LIBDIR=%LIBDIR%\MD"
if "%1"=="MDd" set "SUFFIX=d"
if "%1"=="MTd" set "SUFFIX=d"
if defined SUFFIX set "OPT=/Od"
if not exist "%OBJ%" mkdir "%OBJ%"
if not exist "%LIBDIR%" mkdir "%LIBDIR%"
pushd "%OBJ%"
for %%F in ("%SOURCE%\source\SoundTouch\*.cpp") do (
    cl /nologo /c /EHsc %OPT% /Z7 /%1 /DSOUNDTOUCH_FLOAT_SAMPLES=1 /DSOUNDTOUCH_PREVENT_CLICK_AT_RATE_CROSSOVER=1 /I"%SOURCE%\include" /Fo"%OBJ%\%%~nF.obj" "%%F"
    if errorlevel 1 exit /b 1
)
cl /nologo /c /EHsc %OPT% /Z7 /%1 /DDLL_EXPORTS /DSOUNDTOUCH_FLOAT_SAMPLES=1 /DSOUNDTOUCH_PREVENT_CLICK_AT_RATE_CROSSOVER=1 /I"%SOURCE%\include" /Fo"%OBJ%\SoundTouchDLL.obj" "%SOURCE%\source\SoundTouchDLL\SoundTouchDLL.cpp"
if errorlevel 1 exit /b 1
link /nologo /DLL /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF /OUT:"%LIBDIR%\SoundTouchDLL%SUFFIX%.dll" /IMPLIB:"%LIBDIR%\SoundTouchDLL%BITS%%SUFFIX%.lib" /PDB:"%LIBDIR%\SoundTouchDLL%SUFFIX%.pdb" *.obj
if errorlevel 1 exit /b 1
popd
exit /b 0