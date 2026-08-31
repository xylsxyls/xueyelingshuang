@echo off
setlocal EnableExtensions DisableDelayedExpansion

set "TCP_SERVICE_NAME=vpn-todesk-server"
set "UDP_TASK_NAME=vpn-todesk-server-udp"
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
set "VPN_TODESK_UDP_TASK_NAME=%UDP_TASK_NAME%"
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

echo Uninstalling the ToDesk TLS Proxy server files, TCP service, and UDP task...
call :validate_owned_tcp_service_if_present
if errorlevel 1 exit /b 1
call :validate_owned_udp_task_if_present
if errorlevel 1 exit /b 1

call :stop_tcp_service
if errorlevel 1 exit /b 1
call :stop_udp_task
if errorlevel 1 exit /b 1

call :delete_tcp_service
if errorlevel 1 exit /b 1
call :delete_udp_task
if errorlevel 1 exit /b 1

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

:validate_owned_udp_task_if_present
schtasks.exe /Query /TN "%UDP_TASK_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Scheduled task %UDP_TASK_NAME% is not installed; continuing.
    exit /b 0
)
if exist "%MARKER%" exit /b 0
echo Error: scheduled task %UDP_TASK_NAME% exists, but this package marker is missing.
echo Refusing to delete an unknown scheduled task.
exit /b 1

:stop_tcp_service
sc.exe query "%TCP_SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Service %TCP_SERVICE_NAME% is not installed; no stop needed.
    exit /b 0
)
sc.exe query "%TCP_SERVICE_NAME%" | findstr.exe /I /C:"STOPPED" >nul 2>&1
if not errorlevel 1 (
    echo Service %TCP_SERVICE_NAME% is already stopped; continuing.
    exit /b 0
)
echo Stopping %TCP_SERVICE_NAME%...
sc.exe stop "%TCP_SERVICE_NAME%" >nul 2>&1
call :wait_tcp_service_stopped
exit /b 0

:wait_tcp_service_stopped
for /L %%I in (1,1,20) do (
    sc.exe query "%TCP_SERVICE_NAME%" | findstr.exe /I /C:"STOPPED" >nul 2>&1
    if not errorlevel 1 exit /b 0
    ping.exe -n 2 127.0.0.1 >nul
)
echo Notice: TCP service did not report STOPPED before deletion; continuing.
exit /b 0

:stop_udp_task
schtasks.exe /Query /TN "%UDP_TASK_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Scheduled task %UDP_TASK_NAME% is not installed; no stop needed.
    exit /b 0
)
call :hysteria_process_is_running
if errorlevel 1 (
    echo UDP/Hysteria task is already stopped; continuing.
    exit /b 0
)
echo Stopping %UDP_TASK_NAME%...
schtasks.exe /End /TN "%UDP_TASK_NAME%" >nul 2>&1
call :wait_hysteria_stopped
if errorlevel 1 (
    echo Warning: Hysteria process did not stop quickly; terminating only this package's Hysteria process.
    call :kill_hysteria_processes
    call :wait_hysteria_stopped
)
if errorlevel 1 (
    echo Error: Hysteria process is still running after termination attempt.
    exit /b 1
)
exit /b 0

:wait_hysteria_stopped
for /L %%I in (1,1,20) do (
    call :hysteria_process_is_running
    if errorlevel 1 exit /b 0
    ping.exe -n 2 127.0.0.1 >nul
)
echo Notice: Hysteria process did not exit before deletion.
exit /b 1

:delete_tcp_service
sc.exe query "%TCP_SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Service %TCP_SERVICE_NAME% is already absent.
    exit /b 0
)
echo Deleting %TCP_SERVICE_NAME%...
sc.exe delete "%TCP_SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    sc.exe query "%TCP_SERVICE_NAME%" >nul 2>&1
    if not errorlevel 1 (
        echo Warning: Windows did not delete %TCP_SERVICE_NAME% immediately.
        echo A reboot may be required if the service is marked for deletion.
    )
)
exit /b 0

:delete_udp_task
schtasks.exe /Query /TN "%UDP_TASK_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Scheduled task %UDP_TASK_NAME% is already absent.
    exit /b 0
)
echo Deleting %UDP_TASK_NAME%...
schtasks.exe /Delete /TN "%UDP_TASK_NAME%" /F >nul
if errorlevel 1 (
    echo Warning: Windows did not delete %UDP_TASK_NAME% immediately.
)
exit /b 0

:hysteria_process_is_running
powershell.exe -NoProfile -NonInteractive -Command "$exe=$env:VPN_TODESK_HYSTERIA_EXE; $conf=$env:VPN_TODESK_UDP_CONF; $ps=Get-WmiObject -Class Win32_Process -Filter 'Name=''hysteria-windows-amd64.exe''' -ErrorAction SilentlyContinue | Where-Object { $_.ExecutablePath -and [string]::Equals($_.ExecutablePath,$exe,[StringComparison]::OrdinalIgnoreCase) -and $_.CommandLine -and $_.CommandLine.IndexOf($conf,[StringComparison]::OrdinalIgnoreCase) -ge 0 }; if(@($ps).Count -gt 0){exit 0}; exit 1"
exit /b %ERRORLEVEL%

:kill_hysteria_processes
powershell.exe -NoProfile -NonInteractive -Command "$exe=$env:VPN_TODESK_HYSTERIA_EXE; $conf=$env:VPN_TODESK_UDP_CONF; Get-WmiObject -Class Win32_Process -Filter 'Name=''hysteria-windows-amd64.exe''' -ErrorAction SilentlyContinue | Where-Object { $_.ExecutablePath -and [string]::Equals($_.ExecutablePath,$exe,[StringComparison]::OrdinalIgnoreCase) -and $_.CommandLine -and $_.CommandLine.IndexOf($conf,[StringComparison]::OrdinalIgnoreCase) -ge 0 } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }"
exit /b %ERRORLEVEL%

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
