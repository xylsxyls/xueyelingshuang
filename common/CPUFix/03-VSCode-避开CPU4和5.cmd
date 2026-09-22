@echo off
setlocal
set "CPU_FIX_PS=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if exist "%SystemRoot%\Sysnative\WindowsPowerShell\v1.0\powershell.exe" set "CPU_FIX_PS=%SystemRoot%\Sysnative\WindowsPowerShell\v1.0\powershell.exe"
"%CPU_FIX_PS%" -NoProfile -ExecutionPolicy Bypass -File "%~dp0cpu-switch.ps1" -App VSCode -Mode Avoid
exit /b %errorlevel%
