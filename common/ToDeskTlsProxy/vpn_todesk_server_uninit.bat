@echo off
setlocal EnableExtensions DisableDelayedExpansion

set "SERVICE_NAME=vpn-todesk-server"
set "ROOT=%~dp0"
set "ROOT_DIR=%ROOT:~0,-1%"
set "EXPECTED_ROOT=C:\ProgramData\ToDeskTlsProxy"
set "CONF=%ROOT%stunnel-server.conf"
set "MARKER=%ROOT%.vpn-todesk-server-initialized"
set "FIREWALL_NAME=ToDeskTlsProxy-mTLS-SOCKS-52030"
set "VPN_TODESK_SERVICE_NAME=%SERVICE_NAME%"
set "VPN_TODESK_CONF=%CONF%"

if /I "%~1"=="-h" goto :help
if /I "%~1"=="--help" goto :help
if not "%~1"=="" goto :usage_error

call :require_admin
if errorlevel 1 exit /b 1

if /I not "%ROOT_DIR%"=="%EXPECTED_ROOT%" (
    echo Error: run this BAT from %EXPECTED_ROOT%.
    echo Current script directory: %ROOT_DIR%
    exit /b 1
)

echo Uninstalling the ToDesk TLS Proxy server files and service...
call :validate_owned_service_if_present
if errorlevel 1 exit /b 1

call :stop_project_service
if errorlevel 1 exit /b 1

call :delete_project_service
if errorlevel 1 exit /b 1

echo Removing the dedicated Windows Firewall rule, if it exists...
netsh.exe advfirewall firewall delete rule name="%FIREWALL_NAME%" >nul 2>&1

echo Scheduling package directory removal...
call :schedule_package_removal
if errorlevel 1 exit /b 1

echo.
echo Server uninstall completed.
echo stunnel itself was intentionally kept installed.
echo Tencent Cloud security-group rules are not changed by this script.
echo If C:\ProgramData\ToDeskTlsProxy still exists after a few seconds, close
echo any Command Prompt currently inside that directory and remove it manually.
exit /b 0

:help
echo Usage: vpn_todesk_server_uninit.bat
echo Run it from an Administrator Command Prompt in C:\ProgramData\ToDeskTlsProxy.
exit /b 0

:usage_error
echo Usage: vpn_todesk_server_uninit.bat
exit /b 2

:require_admin
fltmc.exe >nul 2>&1
if errorlevel 1 (
    echo Error: run this BAT from an Administrator Command Prompt.
    exit /b 1
)
exit /b 0

:validate_owned_service_if_present
sc.exe query "%SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Service %SERVICE_NAME% is not installed; continuing.
    exit /b 0
)
if exist "%MARKER%" exit /b 0
powershell.exe -NoProfile -NonInteractive -Command "$ErrorActionPreference='Stop'; $path='Registry::HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\'+$env:VPN_TODESK_SERVICE_NAME; $item=Get-ItemProperty -LiteralPath $path; if(([string]$item.ImagePath).IndexOf($env:VPN_TODESK_CONF,[StringComparison]::OrdinalIgnoreCase) -lt 0){exit 1}; exit 0"
if errorlevel 1 (
    echo Error: service %SERVICE_NAME% exists, but it is not marked as this package's service.
    echo Refusing to delete an unknown Windows service.
    exit /b 1
)
exit /b 0

:stop_project_service
sc.exe query "%SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Service %SERVICE_NAME% is not installed; no stop needed.
    exit /b 0
)
sc.exe query "%SERVICE_NAME%" | findstr.exe /I /C:"STOPPED" >nul 2>&1
if not errorlevel 1 (
    echo Service %SERVICE_NAME% is already stopped; continuing.
    exit /b 0
)
echo Stopping %SERVICE_NAME%...
sc.exe stop "%SERVICE_NAME%" >nul 2>&1
call :wait_service_stopped
exit /b 0

:wait_service_stopped
for /L %%I in (1,1,20) do (
    sc.exe query "%SERVICE_NAME%" | findstr.exe /I /C:"STOPPED" >nul 2>&1
    if not errorlevel 1 exit /b 0
    ping.exe -n 2 127.0.0.1 >nul
)
echo Notice: service did not report STOPPED before deletion; continuing.
exit /b 0

:delete_project_service
sc.exe query "%SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Service %SERVICE_NAME% is already absent.
    exit /b 0
)
echo Deleting %SERVICE_NAME%...
sc.exe delete "%SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    sc.exe query "%SERVICE_NAME%" >nul 2>&1
    if not errorlevel 1 (
        echo Warning: Windows did not delete %SERVICE_NAME% immediately.
        echo A reboot may be required if the service is marked for deletion.
    )
)
exit /b 0

:schedule_package_removal
cd /d "C:\ProgramData" >nul 2>&1
set "CLEANUP_BAT=%TEMP%\vpn_todesk_server_uninit_%RANDOM%%RANDOM%.bat"
(
    echo @echo off
    echo cd /d "C:\ProgramData" ^>nul 2^>nul
    echo for /L %%%%I in ^(1,1,20^) do ^(
    echo     rmdir /S /Q "%ROOT_DIR%" ^>nul 2^>nul
    echo     if not exist "%ROOT_DIR%" goto done
    echo     ping.exe -n 2 127.0.0.1 ^>nul
    echo ^)
    echo echo Warning: could not remove "%ROOT_DIR%".
    echo :done
    echo del /F /Q "%%~f0" ^>nul 2^>nul
) >"%CLEANUP_BAT%"
if not exist "%CLEANUP_BAT%" (
    echo Error: could not create cleanup helper.
    exit /b 1
)
start "" /min cmd.exe /c "%CLEANUP_BAT%"
exit /b 0
