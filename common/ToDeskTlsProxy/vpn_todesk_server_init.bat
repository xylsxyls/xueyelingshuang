@echo off
setlocal EnableExtensions DisableDelayedExpansion

set "SCRIPT_VERSION=2026-08-29-hysteria-udp-v13"
set "TCP_SERVICE_NAME=vpn-todesk-server"
set "UDP_TASK_NAME=vpn-todesk-server-udp"
set "ROOT=%~dp0"
set "ROOT_DIR=%ROOT:~0,-1%"
set "EXPECTED_ROOT=C:\ProgramData\ToDeskTlsProxy"
set "TCP_CONF=%ROOT%stunnel-server.conf"
set "UDP_CONF=%ROOT%hysteria-server.yaml"
set "UDP_RUNNER=%ROOT%vpn_todesk_hysteria_udp_loop.bat"
set "SERVER_CERT=%ROOT%certs\server-fullchain.pem"
set "CLIENT_CA=%ROOT%certs\client-ca.crt"
set "SERVER_KEY=%ROOT%private\server.key"
set "HYSTERIA_EXE=%ROOT%hysteria-windows-amd64.exe"
set "CONTROL=%ROOT%vpn_todesk_server.bat"
set "UNINIT=%ROOT%vpn_todesk_server_uninit.bat"
set "INSTALL_GUIDE=%ROOT%WINDOWS_INSTALL.txt"
set "SHA_FILE=%ROOT%SHA256SUMS.txt"
set "PACKAGE_MARKER=%ROOT%PACKAGE_GENERATED.txt"
set "MARKER=%ROOT%.vpn-todesk-server-initialized"
set "TCP_FIREWALL_NAME=ToDeskTlsProxy-mTLS-SOCKS-52030"
set "UDP_FIREWALL_NAME=ToDeskTlsProxy-Hysteria-UDP-52030"
set "PORT=52030"
set "UDP_LOG_FILE=%ROOT%logs\hysteria-server.log"
set "UDP_TASK_COMMAND=cmd.exe /c %UDP_RUNNER%"
set "STUNNEL_DOWNLOAD_PAGE=https://www.stunnel.org/downloads.html"
set "STUNNEL_LATEST_URL=https://www.stunnel.org/downloads/stunnel-latest-win64-installer.exe"
set "STUNNEL_SHA_URL=https://www.stunnel.org/downloads/stunnel-latest-win64-installer.exe.sha256"
set "VPN_TODESK_TCP_SERVICE_NAME=%TCP_SERVICE_NAME%"
set "VPN_TODESK_UDP_TASK_NAME=%UDP_TASK_NAME%"
set "VPN_TODESK_TCP_CONF=%TCP_CONF%"
set "VPN_TODESK_UDP_CONF=%UDP_CONF%"
set "VPN_TODESK_UDP_RUNNER=%UDP_RUNNER%"
set "VPN_TODESK_HYSTERIA_EXE=%HYSTERIA_EXE%"
set "VPN_TODESK_ROOT_DIR=%ROOT_DIR%"
set "VPN_TODESK_SHA_FILE=%SHA_FILE%"
set "VPN_TODESK_PACKAGE_MARKER=%PACKAGE_MARKER%"
set "VPN_TODESK_PORT=%PORT%"

if /I "%~1"=="-h" goto :help
if /I "%~1"=="--help" goto :help
if not "%~1"=="" goto :usage_error

call :require_admin
if errorlevel 1 exit /b 1

echo ToDesk TLS Proxy server init version: %SCRIPT_VERSION%

if /I not "%ROOT_DIR%"=="%EXPECTED_ROOT%" (
    echo Error: copy all package contents to %EXPECTED_ROOT% first.
    echo Current script directory: %ROOT_DIR%
    exit /b 1
)

for %%F in ("%TCP_CONF%" "%UDP_CONF%" "%UDP_RUNNER%" "%SERVER_CERT%" "%CLIENT_CA%" "%SERVER_KEY%" "%HYSTERIA_EXE%" "%CONTROL%" "%UNINIT%" "%INSTALL_GUIDE%" "%SHA_FILE%" "%PACKAGE_MARKER%") do (
    if not exist "%%~fF" (
        echo Error: missing required package file: %%~fF
        exit /b 1
    )
)

echo Checking the copied package directory...
call :lock_and_verify_acls
if errorlevel 1 (
    echo Error: failed to validate the package directory.
    exit /b 1
)

call :validate_package_marker
if errorlevel 1 (
    echo Error: invalid generated-package marker.
    exit /b 1
)

echo Verifying the copied package checksums...
call :verify_package_hashes
if errorlevel 1 (
    echo Error: package checksum verification failed.
    echo Copy a newly generated windows-server-package again.
    exit /b 1
)

if not exist "%ROOT%logs\" mkdir "%ROOT%logs" >nul 2>&1
if not exist "%ROOT%logs\" (
    echo Error: could not create the logs directory.
    exit /b 1
)

call :find_stunnel
if not defined STUNNEL_EXE (
    echo.
    echo stunnel is not installed.
    echo Download the official Win64 installer from:
    echo   %STUNNEL_DOWNLOAD_PAGE%
    echo Direct latest Win64 installer:
    echo   %STUNNEL_LATEST_URL%
    echo Matching SHA-256 file:
    echo   %STUNNEL_SHA_URL%
    echo After downloading, compare the published hash with:
    echo   certutil -hashfile stunnel-latest-win64-installer.exe SHA256
    echo Then run the installer with its default installation directory.
    set "INSTALL_CONFIRM="
    set /P "INSTALL_CONFIRM=Install stunnel, then press Enter to continue: " <con
    call :find_stunnel
)
if not defined STUNNEL_EXE (
    echo Error: stunnel.exe and tstunnel.exe were not found after confirmation.
    echo Both are expected under Program Files\stunnel\bin.
    exit /b 1
)

echo Found stunnel: %STUNNEL_EXE%
"%TSTUNNEL_EXE%" -version
if errorlevel 1 (
    echo Error: this stunnel command-line build cannot run on the server.
    exit /b 1
)
set "VPN_TODESK_STUNNEL_EXE=%STUNNEL_EXE%"

echo Found Hysteria: %HYSTERIA_EXE%
"%HYSTERIA_EXE%" version
if errorlevel 1 (
    echo Error: the included Hysteria executable cannot run on the server.
    exit /b 1
)

set "MARKER_VERSION="
if exist "%MARKER%" (
    call :validate_existing_marker_contents
    if errorlevel 1 (
        echo Error: the existing project ownership marker is invalid.
        echo Refusing to change any Windows service or scheduled task.
        exit /b 1
    )
)

sc.exe query "stunnel" >nul 2>&1
if not errorlevel 1 echo Notice: existing service "stunnel" was detected and will remain unchanged.

call :install_or_reuse_tcp_service
if errorlevel 1 exit /b 1

call :install_or_recreate_udp_task
if errorlevel 1 exit /b 1

call :write_marker_v3
if errorlevel 1 (
    echo Error: could not write the project ownership marker.
    exit /b 1
)
call :validate_marker_v3
if errorlevel 1 (
    echo Error: could not validate the project ownership marker.
    exit /b 1
)

echo Creating or validating the dedicated Windows Firewall rules...
netsh.exe advfirewall firewall delete rule name="%TCP_FIREWALL_NAME%" >nul 2>&1
netsh.exe advfirewall firewall add rule name="%TCP_FIREWALL_NAME%" dir=in action=allow protocol=TCP localport=%PORT% program="%STUNNEL_EXE%" profile=any
if errorlevel 1 (
    echo Error: the dedicated TCP Windows Firewall rule is missing or unsafe.
    exit /b 1
)
netsh.exe advfirewall firewall delete rule name="%UDP_FIREWALL_NAME%" >nul 2>&1
netsh.exe advfirewall firewall add rule name="%UDP_FIREWALL_NAME%" dir=in action=allow protocol=UDP localport=%PORT% program="%HYSTERIA_EXE%" profile=any
if errorlevel 1 (
    echo Error: the dedicated UDP Windows Firewall rule is missing or unsafe.
    exit /b 1
)

echo.
echo Tencent Cloud console action is required:
echo   Open the CVM security group attached to 175.24.198.162.
echo   Add or confirm these inbound rules:
echo     1. protocol TCP, port 52030
echo     2. protocol UDP, port 52030
echo   Temporary source: 0.0.0.0/0. Restrict it later when possible.
set "TENCENT_CONFIRM="
set /P "TENCENT_CONFIRM=Finish the Tencent Cloud rules, then press Enter to continue: " <con

call "%CONTROL%" start all
if errorlevel 1 (
    echo Error: server initialization finished, but one of the project modes did not start.
    exit /b 1
)

echo.
echo Server initialization completed.
echo TCP service: %TCP_SERVICE_NAME%
echo UDP task: %UDP_TASK_NAME%
echo Control commands:
echo   vpn_todesk_server.bat start tcp
echo   vpn_todesk_server.bat start udp
echo   vpn_todesk_server.bat start all
echo   vpn_todesk_server.bat stop tcp
echo   vpn_todesk_server.bat stop udp
echo   vpn_todesk_server.bat stop all
echo   vpn_todesk_server.bat status all
echo   vpn_todesk_server.bat logs tcp
echo   vpn_todesk_server_uninit.bat
exit /b 0

:help
echo Usage: vpn_todesk_server_init.bat
echo Run it once from an Administrator Command Prompt after copying the package.
exit /b 0

:usage_error
echo Usage: vpn_todesk_server_init.bat
exit /b 2

:require_admin
fltmc.exe >nul 2>&1
if errorlevel 1 (
    echo Error: run this BAT from an Administrator Command Prompt.
    exit /b 1
)
exit /b 0

:verify_package_hashes
powershell.exe -NoProfile -NonInteractive -Command "$ErrorActionPreference='Stop'; $root=[IO.Path]::GetFullPath($env:VPN_TODESK_ROOT_DIR); $expected=@('stunnel-server.conf','hysteria-server.yaml','vpn_todesk_hysteria_udp_loop.bat','hysteria-windows-amd64.exe','certs/server-fullchain.pem','certs/client-ca.crt','private/server.key','vpn_todesk_server_init.bat','vpn_todesk_server.bat','vpn_todesk_server_uninit.bat','WINDOWS_INSTALL.txt','PACKAGE_GENERATED.txt'); $map=@{}; foreach($line in Get-Content -LiteralPath $env:VPN_TODESK_SHA_FILE){if($line -notmatch '^([0-9a-fA-F]{64})\s+\*?(.+)$'){throw 'Invalid SHA256SUMS line'}; $rel=$matches[2].Replace('\','/'); if($map.ContainsKey($rel)){throw 'Duplicate SHA256SUMS entry'}; $map[$rel]=$matches[1].ToLowerInvariant()}; if($map.Count -ne $expected.Count){throw 'Unexpected SHA256SUMS entry count'}; $prefix=$root.TrimEnd('\')+'\'; $sha=[Security.Cryptography.SHA256]::Create(); try{foreach($rel in $expected){if(-not $map.ContainsKey($rel)){throw ('Missing hash for '+$rel)}; $full=[IO.Path]::GetFullPath((Join-Path $root $rel)); if(-not $full.StartsWith($prefix,[StringComparison]::OrdinalIgnoreCase)){throw 'Unsafe checksum path'}; $stream=[IO.File]::OpenRead($full); try{$actual=[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','').ToLowerInvariant()} finally{$stream.Dispose()}; if($actual -ne $map[$rel]){throw ('Hash mismatch: '+$rel)}}} finally{$sha.Dispose()}; exit 0"
exit /b %ERRORLEVEL%

:validate_package_marker
powershell.exe -NoProfile -NonInteractive -Command "$ErrorActionPreference='Stop'; $expected='This directory was generated by vpn_todesk_init.sh.'; $text=[IO.File]::ReadAllText($env:VPN_TODESK_PACKAGE_MARKER); if($text.Length -gt 0 -and [int][char]$text[0] -eq 65279){$text=$text.Substring(1)}; if($text.Trim() -cne $expected){throw 'Invalid generated-package marker'}; exit 0"
exit /b %ERRORLEVEL%

:validate_existing_marker_contents
findstr.exe /L /X /C:"ToDeskTlsProxyServerV3" "%MARKER%" >nul 2>&1
if not errorlevel 1 (
    call :validate_marker_v3
    if errorlevel 1 exit /b 1
    set "MARKER_VERSION=V3"
    exit /b 0
)
findstr.exe /L /X /C:"ToDeskTlsProxyServerV2" "%MARKER%" >nul 2>&1
if not errorlevel 1 (
    call :validate_marker_v2
    if errorlevel 1 exit /b 1
    set "MARKER_VERSION=V2"
    exit /b 0
)
exit /b 1

:validate_marker_v2
findstr.exe /L /X /C:"SERVICE_NAME=%TCP_SERVICE_NAME%" "%MARKER%" >nul 2>&1
if errorlevel 1 exit /b 1
findstr.exe /L /X /C:"CONFIG=%TCP_CONF%" "%MARKER%" >nul 2>&1
if errorlevel 1 exit /b 1
findstr.exe /L /X /C:"STUNNEL_EXE=%STUNNEL_EXE%" "%MARKER%" >nul 2>&1
if errorlevel 1 exit /b 1
exit /b 0

:validate_marker_v3
findstr.exe /L /X /C:"ToDeskTlsProxyServerV3" "%MARKER%" >nul 2>&1
if errorlevel 1 exit /b 1
findstr.exe /L /X /C:"TCP_SERVICE_NAME=%TCP_SERVICE_NAME%" "%MARKER%" >nul 2>&1
if errorlevel 1 exit /b 1
findstr.exe /L /X /C:"UDP_TASK_NAME=%UDP_TASK_NAME%" "%MARKER%" >nul 2>&1
if errorlevel 1 exit /b 1
findstr.exe /L /X /C:"STUNNEL_EXE=%STUNNEL_EXE%" "%MARKER%" >nul 2>&1
if errorlevel 1 exit /b 1
findstr.exe /L /X /C:"TCP_CONFIG=%TCP_CONF%" "%MARKER%" >nul 2>&1
if errorlevel 1 exit /b 1
findstr.exe /L /X /C:"HYSTERIA_EXE=%HYSTERIA_EXE%" "%MARKER%" >nul 2>&1
if errorlevel 1 exit /b 1
findstr.exe /L /X /C:"UDP_CONFIG=%UDP_CONF%" "%MARKER%" >nul 2>&1
if errorlevel 1 exit /b 1
findstr.exe /L /X /C:"UDP_RUNNER=%UDP_RUNNER%" "%MARKER%" >nul 2>&1
if errorlevel 1 exit /b 1
exit /b 0

:write_marker_v3
>"%MARKER%" echo ToDeskTlsProxyServerV3
>>"%MARKER%" echo TCP_SERVICE_NAME=%TCP_SERVICE_NAME%
>>"%MARKER%" echo UDP_TASK_NAME=%UDP_TASK_NAME%
>>"%MARKER%" echo STUNNEL_EXE=%STUNNEL_EXE%
>>"%MARKER%" echo TCP_CONFIG=%TCP_CONF%
>>"%MARKER%" echo HYSTERIA_EXE=%HYSTERIA_EXE%
>>"%MARKER%" echo UDP_CONFIG=%UDP_CONF%
>>"%MARKER%" echo UDP_RUNNER=%UDP_RUNNER%
exit /b %ERRORLEVEL%

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

:lock_and_verify_acls
powershell.exe -NoProfile -NonInteractive -Command "$ErrorActionPreference='Stop'; $item=Get-Item -LiteralPath $env:VPN_TODESK_ROOT_DIR -Force; if(($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0){throw 'The package root must not be a reparse point'}; exit 0"
if errorlevel 1 (
    echo Error: could not inspect the package directory.
    exit /b 1
)
echo   ACL hardening is skipped on this Windows Server 2012 profile.
echo Keep C:\ProgramData\ToDeskTlsProxy accessible only to Administrators.
exit /b 0

:install_or_reuse_tcp_service
sc.exe query "%TCP_SERVICE_NAME%" >nul 2>&1
if errorlevel 1 goto :create_tcp_service
if "%MARKER_VERSION%"=="" (
    echo Error: a service named %TCP_SERVICE_NAME% already exists without this project's marker.
    echo Refusing to stop, reconfigure, or replace it.
    exit /b 1
)
call :tcp_service_identity_is_expected
if errorlevel 1 (
    echo Error: the existing %TCP_SERVICE_NAME% service no longer matches this project.
    exit /b 1
)
echo Reusing the validated TCP service %TCP_SERVICE_NAME%.
exit /b 0

:create_tcp_service
echo Installing the dedicated %TCP_SERVICE_NAME% Windows service...
sc.exe create "%TCP_SERVICE_NAME%" binPath= "%STUNNEL_EXE% -service %TCP_CONF%" DisplayName= "ToDesk TLS Proxy Server"
if errorlevel 1 (
    echo Error: dedicated TCP Windows service creation failed.
    exit /b 1
)
sc.exe description "%TCP_SERVICE_NAME%" "Dedicated stunnel service for the ToDesk TLS proxy" >nul 2>&1
sc.exe config "%TCP_SERVICE_NAME%" depend= Tcpip >nul 2>&1
sc.exe query "%TCP_SERVICE_NAME%" >nul 2>&1
if errorlevel 1 (
    echo Error: Windows reported success but the TCP service was not found.
    exit /b 1
)
call :tcp_service_identity_is_expected
if errorlevel 1 (
    echo Error: the new TCP service registration did not match the expected identity.
    exit /b 1
)
exit /b 0

:install_or_recreate_udp_task
schtasks.exe /Query /TN "%UDP_TASK_NAME%" >nul 2>&1
if errorlevel 1 goto :create_udp_task
if /I "%MARKER_VERSION%"=="V3" (
    echo Recreating the validated UDP scheduled task %UDP_TASK_NAME% with the current command...
    goto :create_udp_task
)
echo Error: a scheduled task named %UDP_TASK_NAME% already exists, but this package has no matching V3 marker.
echo Refusing to replace an unknown scheduled task.
exit /b 1

:create_udp_task
echo Creating the UDP/Hysteria scheduled task %UDP_TASK_NAME%...
schtasks.exe /Create /TN "%UDP_TASK_NAME%" /SC ONSTART /RU SYSTEM /TR "%UDP_TASK_COMMAND%" /F >nul
if errorlevel 1 (
    echo Error: could not create scheduled task %UDP_TASK_NAME%.
    exit /b 1
)
schtasks.exe /Change /TN "%UDP_TASK_NAME%" /DISABLE >nul 2>&1
exit /b 0

:tcp_service_identity_is_expected
powershell.exe -NoProfile -NonInteractive -Command "$ErrorActionPreference='Stop'; $path='Registry::HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\'+$env:VPN_TODESK_TCP_SERVICE_NAME; $item=Get-ItemProperty -LiteralPath $path; $expected=$env:VPN_TODESK_STUNNEL_EXE+' -service '+$env:VPN_TODESK_TCP_CONF; if(-not [string]::Equals([string]$item.ImagePath,$expected,[StringComparison]::OrdinalIgnoreCase)){Write-Error 'Unexpected TCP service ImagePath'; exit 1}; if([int]$item.Type -ne 16){Write-Error 'Unexpected TCP service Type'; exit 1}; if(-not [string]::Equals([string]$item.ObjectName,'LocalSystem',[StringComparison]::OrdinalIgnoreCase)){Write-Error 'Unexpected TCP service account'; exit 1}; if(([int]$item.Start -ne 2) -and ([int]$item.Start -ne 3)){Write-Error 'Unexpected TCP service startup type'; exit 1}; exit 0"
exit /b %ERRORLEVEL%
