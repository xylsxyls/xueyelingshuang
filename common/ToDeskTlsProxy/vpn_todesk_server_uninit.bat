@echo off
setlocal EnableExtensions DisableDelayedExpansion

set "TCP_SERVICE_NAME=vpn-todesk-server"
set "UDP_SERVICE_NAME=vpn-todesk-server-udp"
set "ROOT=%~dp0"
set "ROOT_DIR=%ROOT:~0,-1%"
set "EXPECTED_ROOT=C:\ProgramData\ToDeskTlsProxy"
set "TCP_CONF=%ROOT%stunnel-server.conf"
set "UDP_CONF=%ROOT%hysteria-server.yaml"
set "HYSTERIA_EXE=%ROOT%hysteria-windows-amd64.exe"
set "MARKER=%ROOT%.vpn-todesk-server-initialized"
set "TCP_FIREWALL_NAME=ToDeskTlsProxy-mTLS-SOCKS-52030"
set "UDP_FIREWALL_NAME=ToDeskTlsProxy-Hysteria-UDP-52030"
set "VPN_TODESK_TCP_SERVICE_NAME=%TCP_SERVICE_NAME%"
set "VPN_TODESK_UDP_SERVICE_NAME=%UDP_SERVICE_NAME%"
set "VPN_TODESK_TCP_CONF=%TCP_CONF%"
set "VPN_TODESK_UDP_CONF=%UDP_CONF%"
set "VPN_TODESK_HYSTERIA_EXE=%HYSTERIA_EXE%"

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

echo Uninstalling the ToDesk TLS Proxy server files and services...
call :validate_owned_tcp_service_if_present
if errorlevel 1 exit /b 1
call :validate_owned_udp_service_if_present
if errorlevel 1 exit /b 1

call :stop_project_service "%TCP_SERVICE_NAME%"
if errorlevel 1 exit /b 1
call :stop_project_service "%UDP_SERVICE_NAME%"
if errorlevel 1 exit /b 1
call :kill_hysteria_processes

call :delete_project_service "%TCP_SERVICE_NAME%"
if errorlevel 1 exit /b 1
call :delete_project_service "%UDP_SERVICE_NAME%"
if errorlevel 1 exit /b 1

schtasks.exe /End /TN "%UDP_SERVICE_NAME%" >nul 2>&1
schtasks.exe /Delete /TN "%UDP_SERVICE_NAME%" /F >nul 2>&1

echo Removing the dedicated Windows Firewall rules, if they exist...
netsh.exe advfirewall firewall delete rule name="%TCP_FIREWALL_NAME%" >nul 2>&1
netsh.exe advfirewall firewall delete rule name="%UDP_FIREWALL_NAME%" >nul 2>&1

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

:validate_owned_tcp_service_if_present
sc.exe query "%TCP_SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Service %TCP_SERVICE_NAME% is not installed; continuing.
    exit /b 0
)
if exist "%MARKER%" exit /b 0
powershell.exe -NoProfile -NonInteractive -Command "$ErrorActionPreference='Stop'; $path='Registry::HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\'+$env:VPN_TODESK_TCP_SERVICE_NAME; $item=Get-ItemProperty -LiteralPath $path; if(([string]$item.ImagePath).IndexOf($env:VPN_TODESK_TCP_CONF,[StringComparison]::OrdinalIgnoreCase) -lt 0){exit 1}; exit 0"
if errorlevel 1 (
    echo Error: service %TCP_SERVICE_NAME% exists, but it is not marked as this package's service.
    echo Refusing to delete an unknown Windows service.
    exit /b 1
)
exit /b 0

:validate_owned_udp_service_if_present
sc.exe query "%UDP_SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Service %UDP_SERVICE_NAME% is not installed; continuing.
    exit /b 0
)
if exist "%MARKER%" exit /b 0
powershell.exe -NoProfile -NonInteractive -Command "$ErrorActionPreference='Stop'; $path='Registry::HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\'+$env:VPN_TODESK_UDP_SERVICE_NAME+'\Parameters'; $item=Get-ItemProperty -LiteralPath $path; if(([string]$item.Application).IndexOf($env:VPN_TODESK_HYSTERIA_EXE,[StringComparison]::OrdinalIgnoreCase) -lt 0){exit 1}; if(([string]$item.AppParameters).IndexOf($env:VPN_TODESK_UDP_CONF,[StringComparison]::OrdinalIgnoreCase) -lt 0){exit 1}; exit 0"
if errorlevel 1 (
    echo Error: service %UDP_SERVICE_NAME% exists, but it is not marked as this package's service.
    echo Refusing to delete an unknown Windows service.
    exit /b 1
)
exit /b 0

:stop_project_service
set "STOP_SERVICE=%~1"
sc.exe query "%STOP_SERVICE%" >nul 2>&1
if errorlevel 1 (
    echo Service %STOP_SERVICE% is not installed; no stop needed.
    exit /b 0
)
sc.exe query "%STOP_SERVICE%" | findstr.exe /I /C:"STOPPED" >nul 2>&1
if not errorlevel 1 (
    echo Service %STOP_SERVICE% is already stopped; continuing.
    exit /b 0
)
echo Stopping %STOP_SERVICE%...
sc.exe stop "%STOP_SERVICE%" >nul 2>&1
call :wait_service_stopped "%STOP_SERVICE%"
exit /b 0

:wait_service_stopped
set "WAIT_SERVICE=%~1"
for /L %%I in (1,1,20) do (
    sc.exe query "%WAIT_SERVICE%" | findstr.exe /I /C:"STOPPED" >nul 2>&1
    if not errorlevel 1 exit /b 0
    ping.exe -n 2 127.0.0.1 >nul
)
echo Notice: service %WAIT_SERVICE% did not report STOPPED before deletion; continuing.
exit /b 0

:delete_project_service
set "DELETE_SERVICE=%~1"
sc.exe query "%DELETE_SERVICE%" >nul 2>&1
if errorlevel 1 (
    echo Service %DELETE_SERVICE% is already absent.
    exit /b 0
)
echo Deleting %DELETE_SERVICE%...
sc.exe delete "%DELETE_SERVICE%" >nul 2>&1
if errorlevel 1 (
    sc.exe query "%DELETE_SERVICE%" >nul 2>&1
    if not errorlevel 1 (
        echo Warning: Windows did not delete %DELETE_SERVICE% immediately.
        echo A reboot may be required if the service is marked for deletion.
    )
)
exit /b 0

:kill_hysteria_processes
powershell.exe -NoProfile -NonInteractive -Command "$exe=$env:VPN_TODESK_HYSTERIA_EXE; $conf=$env:VPN_TODESK_UDP_CONF; Get-WmiObject -Class Win32_Process -Filter 'Name=''hysteria-windows-amd64.exe''' -ErrorAction SilentlyContinue | Where-Object { $_.ExecutablePath -and [string]::Equals($_.ExecutablePath,$exe,[StringComparison]::OrdinalIgnoreCase) -and $_.CommandLine -and $_.CommandLine.IndexOf($conf,[StringComparison]::OrdinalIgnoreCase) -ge 0 } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }"
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
