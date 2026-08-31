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
set "NSSM_EXE=%ROOT%nssm.exe"
set "MARKER=%ROOT%.vpn-todesk-server-initialized"
set "TCP_LOG_FILE=%ROOT%logs\stunnel-server.log"
set "UDP_LOG_FILE=%ROOT%logs\hysteria-server.log"
set "PORT=52030"
set "UDP_APP_PARAMETERS=--disable-update-check server -c %UDP_CONF%"
set "VPN_TODESK_TCP_SERVICE_NAME=%TCP_SERVICE_NAME%"
set "VPN_TODESK_UDP_SERVICE_NAME=%UDP_SERVICE_NAME%"
set "VPN_TODESK_TCP_CONF=%TCP_CONF%"
set "VPN_TODESK_UDP_CONF=%UDP_CONF%"
set "VPN_TODESK_HYSTERIA_EXE=%HYSTERIA_EXE%"
set "VPN_TODESK_NSSM_EXE=%NSSM_EXE%"
set "VPN_TODESK_UDP_APP_PARAMETERS=%UDP_APP_PARAMETERS%"
set "VPN_TODESK_ROOT_DIR=%ROOT_DIR%"
set "VPN_TODESK_PORT=%PORT%"
set "VPN_TODESK_TCP_LOG_FILE=%TCP_LOG_FILE%"
set "VPN_TODESK_UDP_LOG_FILE=%UDP_LOG_FILE%"

if /I "%~1"=="-h" goto :help
if /I "%~1"=="--help" goto :help
if "%~1"=="" goto :usage_error
if not "%~3"=="" goto :usage_error

set "ACTION=%~1"
set "MODE=%~2"
if "%MODE%"=="" set "MODE=tcp"

call :require_mode
if errorlevel 1 exit /b 2

if /I "%ACTION%"=="start" goto :start_dispatch
if /I "%ACTION%"=="stop" goto :stop_dispatch
if /I "%ACTION%"=="status" goto :status_dispatch
if /I "%ACTION%"=="logs" goto :logs_dispatch
goto :usage_error

:start_dispatch
call :require_admin
if errorlevel 1 exit /b 1
if /I "%MODE%"=="tcp" (
    call :start_tcp
    exit /b %ERRORLEVEL%
)
if /I "%MODE%"=="udp" (
    call :start_udp
    exit /b %ERRORLEVEL%
)
call :start_tcp
if errorlevel 1 exit /b 1
call :start_udp
exit /b %ERRORLEVEL%

:stop_dispatch
call :require_admin
if errorlevel 1 exit /b 1
if /I "%MODE%"=="tcp" (
    call :stop_tcp
    exit /b %ERRORLEVEL%
)
if /I "%MODE%"=="udp" (
    call :stop_udp
    exit /b %ERRORLEVEL%
)
call :stop_tcp
call :stop_udp
exit /b %ERRORLEVEL%

:status_dispatch
call :require_admin
if errorlevel 1 exit /b 1
if /I "%MODE%"=="tcp" (
    call :status_tcp
    exit /b %ERRORLEVEL%
)
if /I "%MODE%"=="udp" (
    call :status_udp
    exit /b %ERRORLEVEL%
)
call :status_tcp
echo.
call :status_udp
exit /b 0

:logs_dispatch
call :require_admin
if errorlevel 1 exit /b 1
if /I "%MODE%"=="tcp" (
    call :logs_tcp
    exit /b %ERRORLEVEL%
)
if /I "%MODE%"=="udp" (
    call :logs_udp
    exit /b %ERRORLEVEL%
)
call :logs_tcp
echo.
call :logs_udp
exit /b 0

:start_tcp
call :require_managed_tcp_service
if errorlevel 1 exit /b 1

echo Stopping the previous TCP project service, if any...
call :stop_service_if_running "%TCP_SERVICE_NAME%"
if errorlevel 1 exit /b 1

call :tcp_port_is_free
if errorlevel 1 (
    echo Error: TCP port %PORT% is already used by another process.
    echo The script did not terminate that process.
    call :show_tcp_port_owners
    exit /b 1
)

echo Starting a fresh TCP project service...
sc.exe start "%TCP_SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Error: could not start %TCP_SERVICE_NAME%.
    sc.exe queryex "%TCP_SERVICE_NAME%"
    call :show_tcp_logs
    exit /b 1
)
call :wait_service_state "%TCP_SERVICE_NAME%" Running 20
if errorlevel 1 (
    echo Error: %TCP_SERVICE_NAME% did not enter the Running state.
    sc.exe queryex "%TCP_SERVICE_NAME%"
    call :show_tcp_logs
    exit /b 1
)
call :wait_for_tcp_owned_port 15
if errorlevel 1 (
    echo Error: TCP port %PORT% is not owned by %TCP_SERVICE_NAME%.
    call :show_tcp_port_owners
    call :show_tcp_logs
    sc.exe stop "%TCP_SERVICE_NAME%" >nul 2>&1
    call :wait_service_state "%TCP_SERVICE_NAME%" Stopped 10 >nul 2>&1
    exit /b 1
)

echo Enabling automatic startup for %TCP_SERVICE_NAME%...
call :set_service_start_type "%TCP_SERVICE_NAME%" auto
if errorlevel 1 (
    echo Error: could not enable automatic startup for %TCP_SERVICE_NAME%.
    exit /b 1
)

echo ToDesk TLS SOCKS server is ready on TCP %PORT%.
echo TCP automatic startup is enabled. Run vpn_todesk_server.bat stop tcp to disable it.
exit /b 0

:start_udp
call :require_managed_udp_service
if errorlevel 1 exit /b 1

echo Stopping the previous UDP/Hysteria service, if any...
call :stop_service_if_running "%UDP_SERVICE_NAME%"
if errorlevel 1 exit /b 1
call :kill_hysteria_processes

call :udp_port_is_free
if errorlevel 1 (
    echo Error: UDP port %PORT% is already used by another process.
    echo The script did not terminate that process.
    call :show_udp_port_owners
    exit /b 1
)

echo Starting a fresh UDP/Hysteria service...
sc.exe start "%UDP_SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Error: could not start %UDP_SERVICE_NAME%.
    sc.exe queryex "%UDP_SERVICE_NAME%"
    call :show_udp_logs
    exit /b 1
)
call :wait_service_state "%UDP_SERVICE_NAME%" Running 20
if errorlevel 1 (
    echo Error: %UDP_SERVICE_NAME% did not enter the Running state.
    sc.exe queryex "%UDP_SERVICE_NAME%"
    call :show_udp_logs
    exit /b 1
)
call :wait_for_udp_owned_port 15
if errorlevel 1 (
    echo Error: UDP port %PORT% is not owned by the Hysteria process.
    call :show_udp_port_owners
    call :show_udp_logs
    sc.exe stop "%UDP_SERVICE_NAME%" >nul 2>&1
    call :wait_service_state "%UDP_SERVICE_NAME%" Stopped 10 >nul 2>&1
    call :kill_hysteria_processes
    exit /b 1
)

echo Enabling automatic startup for %UDP_SERVICE_NAME%...
call :set_service_start_type "%UDP_SERVICE_NAME%" auto
if errorlevel 1 (
    echo Error: could not enable automatic startup for %UDP_SERVICE_NAME%.
    exit /b 1
)

echo ToDesk Hysteria server is ready on UDP %PORT%.
echo UDP automatic startup is enabled. Run vpn_todesk_server.bat stop udp to disable it.
exit /b 0

:stop_tcp
call :require_managed_tcp_service
if errorlevel 1 exit /b 1
call :stop_service_if_running "%TCP_SERVICE_NAME%"
if errorlevel 1 exit /b 1
call :tcp_port_is_free
if errorlevel 1 (
    echo Warning: TCP port %PORT% is still used by another process.
    call :show_tcp_port_owners
)
echo Disabling automatic startup for %TCP_SERVICE_NAME%...
call :set_service_start_type "%TCP_SERVICE_NAME%" demand
if errorlevel 1 (
    echo Error: could not disable automatic startup for %TCP_SERVICE_NAME%.
    exit /b 1
)
echo TCP service stopped and automatic startup is disabled.
exit /b 0

:stop_udp
call :require_managed_udp_service
if errorlevel 1 exit /b 1
call :stop_service_if_running "%UDP_SERVICE_NAME%"
if errorlevel 1 exit /b 1
call :kill_hysteria_processes
call :udp_port_is_free
if errorlevel 1 (
    echo Warning: UDP port %PORT% is still used by another process.
    call :show_udp_port_owners
)
echo Disabling automatic startup for %UDP_SERVICE_NAME%...
call :set_service_start_type "%UDP_SERVICE_NAME%" demand
if errorlevel 1 (
    echo Error: could not disable automatic startup for %UDP_SERVICE_NAME%.
    exit /b 1
)
echo UDP service stopped and automatic startup is disabled.
exit /b 0

:status_tcp
call :require_managed_tcp_service
if errorlevel 1 exit /b 1
echo TCP service:
sc.exe queryex "%TCP_SERVICE_NAME%"
echo.
sc.exe qc "%TCP_SERVICE_NAME%" | findstr.exe /I /C:"START_TYPE"
call :tcp_port_is_owned_by_service
if errorlevel 1 (
    echo Health: not ready - TCP %PORT% is not owned by %TCP_SERVICE_NAME%.
    call :show_tcp_port_owners
    exit /b 1
)
echo Health: ready - TCP %PORT% is owned by %TCP_SERVICE_NAME%.
exit /b 0

:status_udp
call :require_managed_udp_service
if errorlevel 1 exit /b 1
echo UDP service:
sc.exe queryex "%UDP_SERVICE_NAME%"
echo.
sc.exe qc "%UDP_SERVICE_NAME%" | findstr.exe /I /C:"START_TYPE"
call :udp_port_is_owned_by_hysteria
if errorlevel 1 (
    echo Health: not ready - UDP %PORT% is not owned by the Hysteria process.
    call :show_udp_port_owners
    exit /b 1
)
echo Health: ready - UDP %PORT% is owned by the Hysteria process.
exit /b 0

:logs_tcp
call :require_managed_tcp_service
if errorlevel 1 exit /b 1
call :show_tcp_logs
exit /b %ERRORLEVEL%

:logs_udp
call :require_managed_udp_service
if errorlevel 1 exit /b 1
call :show_udp_logs
exit /b %ERRORLEVEL%

:help
call :usage
exit /b 0

:usage_error
call :usage
exit /b 2

:usage
echo Usage: vpn_todesk_server.bat start^|stop^|status^|logs [tcp^|udp^|all]
echo.
echo   start [tcp]   Start TCP/stunnel mode and enable TCP automatic startup.
echo   start udp     Start UDP/Hysteria mode and enable UDP automatic startup.
echo   start all     Start both server modes and enable automatic startup for both.
echo   stop [tcp]    Stop TCP/stunnel mode and disable TCP automatic startup.
echo   stop udp      Stop UDP/Hysteria mode and disable UDP automatic startup.
echo   stop all      Stop both modes and disable automatic startup for both.
echo   status [mode] Show service state, startup type, and port ownership.
echo   logs [mode]   Show TCP stunnel logs or UDP/Hysteria logs.
echo.
echo The mode argument is optional. The default mode is tcp.
exit /b 0

:require_mode
if /I "%MODE%"=="tcp" exit /b 0
if /I "%MODE%"=="udp" exit /b 0
if /I "%MODE%"=="all" exit /b 0
echo Error: unsupported mode: %MODE%
exit /b 1

:require_admin
fltmc.exe >nul 2>&1
if errorlevel 1 (
    echo Error: run this BAT from an Administrator Command Prompt.
    exit /b 1
)
exit /b 0

:require_package_root
if /I not "%ROOT_DIR%"=="%EXPECTED_ROOT%" (
    echo Error: this control BAT must run from %EXPECTED_ROOT%.
    echo Current script directory: %ROOT_DIR%
    exit /b 1
)
if not exist "%MARKER%" (
    echo Error: server is not initialized. Run vpn_todesk_server_init.bat first.
    exit /b 1
)
findstr.exe /L /X /C:"ToDeskTlsProxyServerV4" "%MARKER%" >nul 2>&1
if errorlevel 1 (
    echo Error: the project ownership marker is invalid or too old.
    echo Re-run vpn_todesk_server_init.bat from the new package.
    exit /b 1
)
exit /b 0

:require_managed_tcp_service
call :require_package_root
if errorlevel 1 exit /b 1
findstr.exe /L /X /C:"TCP_SERVICE_NAME=%TCP_SERVICE_NAME%" "%MARKER%" >nul 2>&1
if errorlevel 1 (
    echo Error: the ownership marker has a different TCP service name.
    exit /b 1
)
findstr.exe /L /X /C:"TCP_CONFIG=%TCP_CONF%" "%MARKER%" >nul 2>&1
if errorlevel 1 (
    echo Error: the ownership marker has a different TCP configuration path.
    exit /b 1
)
call :find_stunnel
if not defined STUNNEL_EXE (
    echo Error: stunnel.exe and tstunnel.exe were not found under Program Files\stunnel\bin.
    exit /b 1
)
findstr.exe /L /X /C:"STUNNEL_EXE=%STUNNEL_EXE%" "%MARKER%" >nul 2>&1
if errorlevel 1 (
    echo Error: the ownership marker has a different stunnel path.
    exit /b 1
)
set "VPN_TODESK_STUNNEL_EXE=%STUNNEL_EXE%"
sc.exe query "%TCP_SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Error: %TCP_SERVICE_NAME% does not exist. Re-run server init.
    exit /b 1
)
call :tcp_service_identity_is_expected
if errorlevel 1 (
    echo Error: TCP service ownership validation failed.
    exit /b 1
)
exit /b 0

:require_managed_udp_service
call :require_package_root
if errorlevel 1 exit /b 1
if not exist "%HYSTERIA_EXE%" (
    echo Error: missing Hysteria executable: %HYSTERIA_EXE%
    exit /b 1
)
if not exist "%NSSM_EXE%" (
    echo Error: missing NSSM executable: %NSSM_EXE%
    exit /b 1
)
if not exist "%UDP_CONF%" (
    echo Error: missing UDP config: %UDP_CONF%
    exit /b 1
)
findstr.exe /L /X /C:"UDP_SERVICE_NAME=%UDP_SERVICE_NAME%" "%MARKER%" >nul 2>&1
if errorlevel 1 (
    echo Error: the ownership marker has a different UDP service name.
    exit /b 1
)
findstr.exe /L /X /C:"NSSM_EXE=%NSSM_EXE%" "%MARKER%" >nul 2>&1
if errorlevel 1 (
    echo Error: the ownership marker has a different NSSM executable path.
    exit /b 1
)
findstr.exe /L /X /C:"HYSTERIA_EXE=%HYSTERIA_EXE%" "%MARKER%" >nul 2>&1
if errorlevel 1 (
    echo Error: the ownership marker has a different Hysteria executable path.
    exit /b 1
)
findstr.exe /L /X /C:"UDP_CONFIG=%UDP_CONF%" "%MARKER%" >nul 2>&1
if errorlevel 1 (
    echo Error: the ownership marker has a different UDP configuration path.
    exit /b 1
)
sc.exe query "%UDP_SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Error: %UDP_SERVICE_NAME% does not exist. Re-run server init.
    exit /b 1
)
call :udp_service_identity_is_expected
if errorlevel 1 (
    echo Error: UDP service ownership validation failed.
    exit /b 1
)
exit /b 0

:find_stunnel
set "STUNNEL_EXE="
set "TSTUNNEL_EXE="
if exist "%ProgramW6432%\stunnel\bin\stunnel.exe" if exist "%ProgramW6432%\stunnel\bin\tstunnel.exe" (
    set "STUNNEL_EXE=%ProgramW6432%\stunnel\bin\stunnel.exe"
    set "TSTUNNEL_EXE=%ProgramW6432%\stunnel\bin\tstunnel.exe"
)
if not defined STUNNEL_EXE if exist "%ProgramFiles%\stunnel\bin\stunnel.exe" if exist "%ProgramFiles%\stunnel\bin\tstunnel.exe" (
    set "STUNNEL_EXE=%ProgramFiles%\stunnel\bin\stunnel.exe"
    set "TSTUNNEL_EXE=%ProgramFiles%\stunnel\bin\tstunnel.exe"
)
if not defined STUNNEL_EXE if exist "%ProgramFiles(x86)%\stunnel\bin\stunnel.exe" if exist "%ProgramFiles(x86)%\stunnel\bin\tstunnel.exe" (
    set "STUNNEL_EXE=%ProgramFiles(x86)%\stunnel\bin\stunnel.exe"
    set "TSTUNNEL_EXE=%ProgramFiles(x86)%\stunnel\bin\tstunnel.exe"
)
if defined STUNNEL_EXE (
    for %%I in ("%STUNNEL_EXE%") do set "STUNNEL_EXE=%%~sI"
    for %%I in ("%TSTUNNEL_EXE%") do set "TSTUNNEL_EXE=%%~sI"
)
exit /b 0

:tcp_service_identity_is_expected
powershell.exe -NoProfile -NonInteractive -Command "$ErrorActionPreference='Stop'; $path='Registry::HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\'+$env:VPN_TODESK_TCP_SERVICE_NAME; $item=Get-ItemProperty -LiteralPath $path; $expected=$env:VPN_TODESK_STUNNEL_EXE+' -service '+$env:VPN_TODESK_TCP_CONF; if(-not [string]::Equals([string]$item.ImagePath,$expected,[StringComparison]::OrdinalIgnoreCase)){Write-Error 'Unexpected TCP service ImagePath'; exit 1}; if([int]$item.Type -ne 16){Write-Error 'Unexpected TCP service Type'; exit 1}; if(-not [string]::Equals([string]$item.ObjectName,'LocalSystem',[StringComparison]::OrdinalIgnoreCase)){Write-Error 'Unexpected TCP service account'; exit 1}; if(([int]$item.Start -ne 2) -and ([int]$item.Start -ne 3)){Write-Error 'Unexpected TCP service startup type'; exit 1}; exit 0"
exit /b %ERRORLEVEL%

:udp_service_identity_is_expected
powershell.exe -NoProfile -NonInteractive -Command "$ErrorActionPreference='Stop'; $svcPath='Registry::HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\'+$env:VPN_TODESK_UDP_SERVICE_NAME; $svc=Get-ItemProperty -LiteralPath $svcPath; if(([string]$svc.ImagePath).IndexOf($env:VPN_TODESK_NSSM_EXE,[StringComparison]::OrdinalIgnoreCase) -lt 0){Write-Error 'Unexpected UDP service wrapper'; exit 1}; if([int]$svc.Type -ne 16){Write-Error 'Unexpected UDP service Type'; exit 1}; if(-not [string]::Equals([string]$svc.ObjectName,'LocalSystem',[StringComparison]::OrdinalIgnoreCase)){Write-Error 'Unexpected UDP service account'; exit 1}; $paramPath=$svcPath+'\Parameters'; $p=Get-ItemProperty -LiteralPath $paramPath; if(-not [string]::Equals([string]$p.Application,$env:VPN_TODESK_HYSTERIA_EXE,[StringComparison]::OrdinalIgnoreCase)){Write-Error 'Unexpected UDP application'; exit 1}; if(-not [string]::Equals([string]$p.AppParameters,$env:VPN_TODESK_UDP_APP_PARAMETERS,[StringComparison]::OrdinalIgnoreCase)){Write-Error 'Unexpected UDP parameters'; exit 1}; if(-not [string]::Equals([string]$p.AppDirectory,$env:VPN_TODESK_ROOT_DIR,[StringComparison]::OrdinalIgnoreCase)){Write-Error 'Unexpected UDP working directory'; exit 1}; exit 0"
exit /b %ERRORLEVEL%

:set_service_start_type
sc.exe config "%~1" start= %~2 >nul
exit /b %ERRORLEVEL%

:service_state_is
set "VPN_TODESK_CHECK_SERVICE=%~1"
set "VPN_TODESK_CHECK_STATE=%~2"
powershell.exe -NoProfile -NonInteractive -Command "$s=Get-Service -Name $env:VPN_TODESK_CHECK_SERVICE -ErrorAction SilentlyContinue; if($s -and $s.Status.ToString() -eq $env:VPN_TODESK_CHECK_STATE){exit 0}; exit 1"
exit /b %ERRORLEVEL%

:wait_service_state
set "WAIT_SERVICE=%~1"
set "WAIT_STATE=%~2"
set "WAIT_COUNT=%~3"
for /L %%I in (1,1,%WAIT_COUNT%) do (
    call :service_state_is "%WAIT_SERVICE%" "%WAIT_STATE%"
    if not errorlevel 1 exit /b 0
    ping.exe -n 2 127.0.0.1 >nul
)
exit /b 1

:stop_service_if_running
set "STOP_SERVICE=%~1"
call :service_state_is "%STOP_SERVICE%" Stopped
if not errorlevel 1 (
    echo Service %STOP_SERVICE% is already stopped; continuing.
    exit /b 0
)
sc.exe stop "%STOP_SERVICE%" >nul 2>&1
if errorlevel 1 (
    call :service_state_is "%STOP_SERVICE%" StopPending
    if errorlevel 1 (
        echo Error: could not request %STOP_SERVICE% stop.
        sc.exe queryex "%STOP_SERVICE%"
        exit /b 1
    )
)
call :wait_service_state "%STOP_SERVICE%" Stopped 20
if errorlevel 1 (
    echo Error: %STOP_SERVICE% did not stop within 20 seconds.
    sc.exe queryex "%STOP_SERVICE%"
    exit /b 1
)
exit /b 0

:tcp_port_is_free
powershell.exe -NoProfile -NonInteractive -Command "$p=[regex]::Escape($env:VPN_TODESK_PORT); foreach($line in netstat -ano -p tcp){ if($line -match ('^\s*TCP\s+\S+:' + $p + '\s+\S+\s+LISTENING\s+\d+\s*$')){exit 1}}; exit 0"
exit /b %ERRORLEVEL%

:udp_port_is_free
powershell.exe -NoProfile -NonInteractive -Command "$p=[regex]::Escape($env:VPN_TODESK_PORT); foreach($line in netstat -ano -p udp){ if($line -match ('^\s*UDP\s+\S+:' + $p + '\s+\*:\*\s+\d+\s*$')){exit 1}}; exit 0"
exit /b %ERRORLEVEL%

:tcp_port_is_owned_by_service
powershell.exe -NoProfile -NonInteractive -Command "$filter='Name='''+$env:VPN_TODESK_TCP_SERVICE_NAME+''''; $s=Get-WmiObject -Class Win32_Service -Filter $filter -ErrorAction SilentlyContinue; if(-not $s -or $s.State -ne 'Running' -or [uint32]$s.ProcessId -eq 0){exit 1}; $pid=[uint32]$s.ProcessId; $p=[regex]::Escape($env:VPN_TODESK_PORT); foreach($line in netstat -ano -p tcp){ if($line -match ('^\s*TCP\s+\S+:' + $p + '\s+\S+\s+LISTENING\s+(\d+)\s*$') -and [uint32]$matches[1] -eq $pid){exit 0}}; exit 1"
exit /b %ERRORLEVEL%

:hysteria_process_is_running
powershell.exe -NoProfile -NonInteractive -Command "$exe=$env:VPN_TODESK_HYSTERIA_EXE; $conf=$env:VPN_TODESK_UDP_CONF; $ps=Get-WmiObject -Class Win32_Process -Filter 'Name=''hysteria-windows-amd64.exe''' -ErrorAction SilentlyContinue | Where-Object { $_.ExecutablePath -and [string]::Equals($_.ExecutablePath,$exe,[StringComparison]::OrdinalIgnoreCase) -and $_.CommandLine -and $_.CommandLine.IndexOf($conf,[StringComparison]::OrdinalIgnoreCase) -ge 0 }; if(@($ps).Count -gt 0){exit 0}; exit 1"
exit /b %ERRORLEVEL%

:udp_port_is_owned_by_hysteria
powershell.exe -NoProfile -NonInteractive -Command "$exe=$env:VPN_TODESK_HYSTERIA_EXE; $conf=$env:VPN_TODESK_UDP_CONF; $ps=Get-WmiObject -Class Win32_Process -Filter 'Name=''hysteria-windows-amd64.exe''' -ErrorAction SilentlyContinue | Where-Object { $_.ExecutablePath -and [string]::Equals($_.ExecutablePath,$exe,[StringComparison]::OrdinalIgnoreCase) -and $_.CommandLine -and $_.CommandLine.IndexOf($conf,[StringComparison]::OrdinalIgnoreCase) -ge 0 }; $ids=@($ps | ForEach-Object {[uint32]$_.ProcessId}); if($ids.Count -eq 0){exit 1}; $p=[regex]::Escape($env:VPN_TODESK_PORT); foreach($line in netstat -ano -p udp){ if($line -match ('^\s*UDP\s+\S+:' + $p + '\s+\*:\*\s+(\d+)\s*$') -and $ids -contains [uint32]$matches[1]){exit 0}}; exit 1"
exit /b %ERRORLEVEL%

:wait_for_tcp_owned_port
set "WAIT_COUNT=%~1"
for /L %%I in (1,1,%WAIT_COUNT%) do (
    call :tcp_port_is_owned_by_service
    if not errorlevel 1 exit /b 0
    ping.exe -n 2 127.0.0.1 >nul
)
exit /b 1

:wait_for_udp_owned_port
set "WAIT_COUNT=%~1"
for /L %%I in (1,1,%WAIT_COUNT%) do (
    call :udp_port_is_owned_by_hysteria
    if not errorlevel 1 exit /b 0
    ping.exe -n 2 127.0.0.1 >nul
)
exit /b 1

:show_tcp_port_owners
netstat.exe -ano -p tcp | findstr.exe /R /C:":%PORT%[ ]" || echo No TCP listener found on %PORT%.
exit /b 0

:show_udp_port_owners
netstat.exe -ano -p udp | findstr.exe /R /C:":%PORT%[ ]" || echo No UDP listener found on %PORT%.
exit /b 0

:show_hysteria_processes
powershell.exe -NoProfile -NonInteractive -Command "Get-WmiObject -Class Win32_Process -Filter 'Name=''hysteria-windows-amd64.exe''' -ErrorAction SilentlyContinue | Select-Object ProcessId,ExecutablePath,CommandLine | Format-List"
exit /b 0

:kill_hysteria_processes
powershell.exe -NoProfile -NonInteractive -Command "$exe=$env:VPN_TODESK_HYSTERIA_EXE; $conf=$env:VPN_TODESK_UDP_CONF; Get-WmiObject -Class Win32_Process -Filter 'Name=''hysteria-windows-amd64.exe''' -ErrorAction SilentlyContinue | Where-Object { $_.ExecutablePath -and [string]::Equals($_.ExecutablePath,$exe,[StringComparison]::OrdinalIgnoreCase) -and $_.CommandLine -and $_.CommandLine.IndexOf($conf,[StringComparison]::OrdinalIgnoreCase) -ge 0 } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }"
exit /b %ERRORLEVEL%

:show_tcp_logs
if not exist "%TCP_LOG_FILE%" (
    echo No TCP server log exists yet: %TCP_LOG_FILE%
    exit /b 0
)
powershell.exe -NoProfile -NonInteractive -Command "Get-Content -LiteralPath $env:VPN_TODESK_TCP_LOG_FILE -Tail 80"
exit /b %ERRORLEVEL%

:show_udp_logs
if not exist "%UDP_LOG_FILE%" (
    echo No UDP server log exists yet: %UDP_LOG_FILE%
    exit /b 0
)
powershell.exe -NoProfile -NonInteractive -Command "Get-Content -LiteralPath $env:VPN_TODESK_UDP_LOG_FILE -Tail 100"
exit /b %ERRORLEVEL%
