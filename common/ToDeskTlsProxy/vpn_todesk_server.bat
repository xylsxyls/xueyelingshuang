@echo off
setlocal EnableExtensions DisableDelayedExpansion

set "SERVICE_NAME=vpn-todesk-server"
set "ROOT=%~dp0"
set "ROOT_DIR=%ROOT:~0,-1%"
set "EXPECTED_ROOT=C:\ProgramData\ToDeskTlsProxy"
set "CONF=%ROOT%stunnel-server.conf"
set "MARKER=%ROOT%.vpn-todesk-server-initialized"
set "LOG_FILE=%ROOT%logs\stunnel-server.log"
set "PORT=52030"
set "VPN_TODESK_SERVICE_NAME=%SERVICE_NAME%"
set "VPN_TODESK_CONF=%CONF%"
set "VPN_TODESK_ROOT_DIR=%ROOT_DIR%"
set "VPN_TODESK_PORT=%PORT%"
set "VPN_TODESK_LOG_FILE=%LOG_FILE%"

if /I "%~1"=="start" goto :start
if /I "%~1"=="stop" goto :stop
if /I "%~1"=="status" goto :status
if /I "%~1"=="logs" goto :logs
if /I "%~1"=="-h" goto :help
if /I "%~1"=="--help" goto :help
goto :usage_error

:start
call :require_admin
if errorlevel 1 exit /b 1
call :require_managed_service
if errorlevel 1 exit /b 1

echo Stopping the previous project service, if any...
call :service_state_is Stopped
if not errorlevel 1 goto :start_port_check
sc.exe stop "%SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    call :service_state_is StopPending
    if errorlevel 1 (
        echo Error: could not request project service stop.
        sc.exe queryex "%SERVICE_NAME%"
        exit /b 1
    )
)
call :wait_for_state Stopped 20
if errorlevel 1 (
    echo Error: the project service did not stop within 20 seconds.
    sc.exe queryex "%SERVICE_NAME%"
    exit /b 1
)

:start_port_check
call :port_is_free
if errorlevel 1 (
    echo Error: TCP port %PORT% is already used by another process.
    echo The script did not terminate that process.
    call :show_port_owners
    exit /b 1
)

echo Starting a fresh project service...
sc.exe start "%SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Error: could not start the project service.
    sc.exe queryex "%SERVICE_NAME%"
    call :show_logs
    exit /b 1
)
call :wait_for_state Running 20
if errorlevel 1 (
    echo Error: the project service did not enter the Running state.
    sc.exe queryex "%SERVICE_NAME%"
    call :show_logs
    exit /b 1
)
call :wait_for_owned_port 15
if errorlevel 1 (
    echo Error: TCP port %PORT% is not owned by the project service process.
    call :show_port_owners
    call :show_logs
    sc.exe stop "%SERVICE_NAME%" >nul 2>&1
    call :wait_for_state Stopped 10 >nul 2>&1
    exit /b 1
)

echo ToDesk TLS SOCKS server is ready on TCP %PORT%.
exit /b 0

:stop
call :require_admin
if errorlevel 1 exit /b 1
call :require_managed_service
if errorlevel 1 exit /b 1

call :service_state_is Stopped
if not errorlevel 1 goto :stop_port_check
sc.exe stop "%SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    call :service_state_is StopPending
    if errorlevel 1 (
        echo Error: could not request project service stop.
        sc.exe queryex "%SERVICE_NAME%"
        exit /b 1
    )
)
call :wait_for_state Stopped 20
if errorlevel 1 (
    echo Error: the project service did not stop within 20 seconds.
    sc.exe queryex "%SERVICE_NAME%"
    exit /b 1
)

:stop_port_check
call :port_is_free
if errorlevel 1 (
    echo Warning: TCP port %PORT% is still used by another process.
    echo The script did not terminate that process.
    call :show_port_owners
)
echo ToDesk TLS SOCKS server stopped.
exit /b 0

:status
call :require_admin
if errorlevel 1 exit /b 1
call :require_managed_service
if errorlevel 1 exit /b 1
sc.exe queryex "%SERVICE_NAME%"
call :port_is_owned_by_service
if errorlevel 1 (
    echo Health: not ready - TCP %PORT% is not owned by the project service.
    call :show_port_owners
    exit /b 1
)
echo Health: ready - TCP %PORT% is owned by the project service.
exit /b 0

:logs
call :require_admin
if errorlevel 1 exit /b 1
call :require_managed_service
if errorlevel 1 exit /b 1
call :show_logs
exit /b %ERRORLEVEL%

:help
call :usage
exit /b 0

:usage_error
call :usage
exit /b 2

:usage
echo Usage: vpn_todesk_server.bat start^|stop^|status^|logs
echo.
echo   start   Stop the previous project service, then start a fresh one.
echo   stop    Stop only the dedicated vpn-todesk-server service.
echo   status  Show service state and verify its ownership of TCP 52030.
echo   logs    Show the latest project stunnel log entries.
exit /b 0

:require_admin
fltmc.exe >nul 2>&1
if errorlevel 1 (
    echo Error: run this BAT from an Administrator Command Prompt.
    exit /b 1
)
exit /b 0

:require_managed_service
if /I not "%ROOT_DIR%"=="%EXPECTED_ROOT%" (
    echo Error: this control BAT must run from %EXPECTED_ROOT%.
    echo Current script directory: %ROOT_DIR%
    exit /b 1
)
if not exist "%MARKER%" (
    echo Error: server is not initialized. Run vpn_todesk_server_init.bat first.
    exit /b 1
)
findstr.exe /L /X /C:"ToDeskTlsProxyServerV2" "%MARKER%" >nul 2>&1
if errorlevel 1 (
    echo Error: the project ownership marker is invalid.
    echo Refusing to control any Windows service.
    exit /b 1
)
findstr.exe /L /X /C:"SERVICE_NAME=%SERVICE_NAME%" "%MARKER%" >nul 2>&1
if errorlevel 1 (
    echo Error: the ownership marker has a different service name.
    exit /b 1
)
findstr.exe /L /X /C:"CONFIG=%CONF%" "%MARKER%" >nul 2>&1
if errorlevel 1 (
    echo Error: the ownership marker has a different configuration path.
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
sc.exe query "%SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Error: the project service does not exist. Re-run server init.
    exit /b 1
)
call :service_identity_is_expected
if errorlevel 1 (
    echo Error: project service ownership validation failed.
    echo Refusing to start, stop, or otherwise control that service.
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

:service_identity_is_expected
powershell.exe -NoProfile -NonInteractive -Command "$ErrorActionPreference='Stop'; $path='Registry::HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\'+$env:VPN_TODESK_SERVICE_NAME; $item=Get-ItemProperty -LiteralPath $path; $expected=$env:VPN_TODESK_STUNNEL_EXE+' -service '+$env:VPN_TODESK_CONF; if(-not [string]::Equals([string]$item.ImagePath,$expected,[StringComparison]::OrdinalIgnoreCase)){Write-Error 'Unexpected service ImagePath'; exit 1}; if([int]$item.Type -ne 16){Write-Error 'Unexpected service Type'; exit 1}; if(-not [string]::Equals([string]$item.ObjectName,'LocalSystem',[StringComparison]::OrdinalIgnoreCase)){Write-Error 'Unexpected service account'; exit 1}; if([int]$item.Start -ne 3){Write-Error 'Unexpected service startup type'; exit 1}; exit 0"
exit /b %ERRORLEVEL%

:service_state_is
powershell.exe -NoProfile -NonInteractive -Command "$s=Get-Service -Name $env:VPN_TODESK_SERVICE_NAME -ErrorAction SilentlyContinue; if($s -and $s.Status.ToString() -eq '%~1'){exit 0}; exit 1"
exit /b %ERRORLEVEL%

:wait_for_state
set "WAIT_STATE=%~1"
set "WAIT_COUNT=%~2"
for /L %%I in (1,1,%WAIT_COUNT%) do (
    call :service_state_is %WAIT_STATE%
    if not errorlevel 1 exit /b 0
    ping.exe -n 2 127.0.0.1 >nul
)
exit /b 1

:port_is_free
powershell.exe -NoProfile -NonInteractive -Command "$p=[int]$env:VPN_TODESK_PORT; if(Get-NetTCPConnection -State Listen -LocalPort $p -ErrorAction SilentlyContinue){exit 1}; exit 0"
exit /b %ERRORLEVEL%

:port_is_owned_by_service
powershell.exe -NoProfile -NonInteractive -Command "$filter='Name='''+$env:VPN_TODESK_SERVICE_NAME+''''; $s=Get-WmiObject -Class Win32_Service -Filter $filter -ErrorAction SilentlyContinue; $p=[int]$env:VPN_TODESK_PORT; $listeners=@(Get-NetTCPConnection -State Listen -LocalPort $p -ErrorAction SilentlyContinue | Where-Object {$_.LocalAddress -eq '0.0.0.0'}); if($s -and $s.State -eq 'Running' -and [uint32]$s.ProcessId -gt 0 -and @($listeners.OwningProcess) -contains [uint32]$s.ProcessId){exit 0}; exit 1"
exit /b %ERRORLEVEL%

:wait_for_owned_port
set "WAIT_COUNT=%~1"
for /L %%I in (1,1,%WAIT_COUNT%) do (
    call :port_is_owned_by_service
    if not errorlevel 1 exit /b 0
    ping.exe -n 2 127.0.0.1 >nul
)
exit /b 1

:show_port_owners
powershell.exe -NoProfile -NonInteractive -Command "$p=[int]$env:VPN_TODESK_PORT; Get-NetTCPConnection -State Listen -LocalPort $p -ErrorAction SilentlyContinue | Select-Object LocalAddress,LocalPort,OwningProcess | Format-Table -AutoSize"
exit /b 0

:show_logs
if not exist "%LOG_FILE%" (
    echo No server log exists yet: %LOG_FILE%
    exit /b 0
)
powershell.exe -NoProfile -NonInteractive -Command "Get-Content -LiteralPath $env:VPN_TODESK_LOG_FILE -Tail 80"
exit /b %ERRORLEVEL%
