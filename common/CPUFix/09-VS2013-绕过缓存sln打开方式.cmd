@echo off
setlocal
set "CPU_FIX_HOST=%SystemRoot%\System32\cscript.exe"
if exist "%SystemRoot%\Sysnative\cscript.exe" set "CPU_FIX_HOST=%SystemRoot%\Sysnative\cscript.exe"
"%CPU_FIX_HOST%" //NoLogo "%~dp0vs2013-launch.vbs" --enable
set "CPU_FIX_RESULT=%errorlevel%"
pause
exit /b %CPU_FIX_RESULT%
