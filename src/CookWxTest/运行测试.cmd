@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\run.ps1" %*
set "cook_test_exit=%errorlevel%"
if "%~1"=="" pause
exit /b %cook_test_exit%