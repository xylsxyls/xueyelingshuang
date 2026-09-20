param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('VSCode','Chrome','ToDesk','All')][string]$App,
    [Parameter(Mandatory=$true)]
    [ValidateSet('Avoid','Restore')][string]$Mode,
    [switch]$Preview,
    [switch]$NoPause,
    [switch]$GuardWorker
)
$ErrorActionPreference = 'Stop'
if ([IntPtr]::Size -ne 8) { throw 'Please use 64-bit PowerShell.' }
if ($GuardWorker) {
    if ($App -eq 'All' -or $Mode -ne 'Avoid' -or $Preview) { throw 'Invalid worker arguments.' }
    $NoPause = $true
}
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
if (-not $Preview -and -not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    try {
        $launchArgs = @('-NoProfile','-ExecutionPolicy','Bypass','-File',('"{0}"' -f $PSCommandPath),'-App',$App,'-Mode',$Mode)
        if ($NoPause) { $launchArgs += '-NoPause' }
        if ($GuardWorker) { $launchArgs += '-GuardWorker' }
        $child = Start-Process -FilePath (Join-Path $PSHOME 'powershell.exe') -ArgumentList $launchArgs -Verb RunAs -Wait -PassThru
        exit $child.ExitCode
    } catch {
        Write-Host ('未执行：' + $_.Exception.Message) -ForegroundColor Red
        if (-not $NoPause) { [void](Read-Host '按回车关闭') }
        exit 1
    }
}

$appInfo = @{
    VSCode = @{ Label='VSCode / Codex'; AvoidScript='01'; RestoreScript='02'; Images=@('Code.exe','codex.exe','codex-code-mode-host.exe') }
    Chrome = @{ Label='Chrome'; AvoidScript='03'; RestoreScript='04'; Images=@('chrome.exe') }
    ToDesk = @{ Label='ToDesk'; AvoidScript='05'; RestoreScript='06'; Images=@('ToDesk.exe') }
}

function Invoke-AppAffinity {
    param([string]$SelectedApp, [string]$SelectedMode, [bool]$PreviewOnly)
    # All means exactly these three applications, never the whole operating system.
    $apps = if ($SelectedApp -eq 'All') { @('VSCode','Chrome','ToDesk') } else { @($SelectedApp) }
    $names = [System.Collections.Generic.List[string]]::new()
    $allowed = [System.Collections.Generic.Dictionary[string,string]]::new([StringComparer]::OrdinalIgnoreCase)
    if ($apps -contains 'VSCode') {
        foreach ($name in @('Code','codex','codex-code-mode-host')) { $names.Add($name) }
        $allowed['C:\Users\Administrator\AppData\Local\Programs\Microsoft VS Code\Code.exe'] = 'VSCode'
        $extensionDir = 'C:\Users\Administrator\.vscode\extensions'
        if (Test-Path -LiteralPath $extensionDir -PathType Container) {
            foreach ($ext in @(Get-ChildItem -LiteralPath $extensionDir -Directory -Filter 'openai.chatgpt-*')) {
                foreach ($exe in @('codex.exe','codex-code-mode-host.exe')) {
                    $allowed[(Join-Path $ext.FullName ('bin\windows-x86_64\' + $exe))] = 'VSCode'
                }
            }
        }
    }
    if ($apps -contains 'Chrome') {
        $names.Add('chrome')
        foreach ($path in @(
            'C:\Users\Administrator\AppData\Local\Google\Chrome\Bin\chrome.exe',
            'C:\Users\Administrator\AppData\Local\Google\Chrome\Application\chrome.exe',
            'C:\Program Files\Google\Chrome\Application\chrome.exe',
            'C:\Program Files (x86)\Google\Chrome\Application\chrome.exe'
        )) { $allowed[$path] = 'Chrome' }
    }
    if ($apps -contains 'ToDesk') {
        $names.Add('ToDesk')
        $allowed['C:\Program Files\ToDesk\ToDesk.exe'] = 'ToDesk'
        $allowed['C:\Program Files (x86)\ToDesk\ToDesk.exe'] = 'ToDesk'
    }
    $seen = [System.Collections.Generic.HashSet[string]]::new()
    $results = [System.Collections.Generic.List[object]]::new()
    $failures = [System.Collections.Generic.List[string]]::new()
    for ($pass = 1; $pass -le 2; $pass++) {
        foreach ($proc in @(Get-Process -Name $names.ToArray() -ErrorAction SilentlyContinue)) {
            try {
                # Keep the handle open so this Process instance cannot target a reused PID.
                $heldHandle = $proc.Handle
                $imagePath = $proc.Path
                if (-not $allowed.ContainsKey($imagePath)) { continue }
                $started = $proc.StartTime.ToUniversalTime().ToString('o')
                if (-not $seen.Add(('{0}|{1}' -f $proc.Id,$started))) { continue }
                $before = $proc.ProcessorAffinity.ToInt64()
                if ($SelectedMode -eq 'Avoid') { $desired = $before -band (-bnot [long]0x30) }
                else { $desired = $before -bor [long]0x30 }
                if ($desired -eq 0) { throw '设置后将没有可用 CPU，已拒绝。' }
                if (-not $PreviewOnly -and $before -ne $desired) { $proc.ProcessorAffinity = [IntPtr]$desired }
                $proc.Refresh()
                $after = $proc.ProcessorAffinity.ToInt64()
                if (-not $PreviewOnly -and $after -ne $desired) { throw '设置后的读取结果不一致。' }
                $results.Add([pscustomobject]@{
                    App=$allowed[$imagePath]; PID=$proc.Id; Process=$proc.ProcessName
                    Before=('0x{0:X}' -f $before); Target=('0x{0:X}' -f $desired)
                    After=('0x{0:X}' -f $after)
                })
            } catch {
                $problem = 'PID {0}: {1}' -f $proc.Id,$_.Exception.Message
                $exited = $false
                try { $exited = $proc.HasExited } catch {}
                if (-not $exited) { $failures.Add($problem) }
            } finally {
                $proc.Dispose()
            }
        }
    }
    return [pscustomobject]@{ Apps=$apps; Results=$results; Failures=$failures }
}

# Each app has independent controls within this Windows login session.
# No files, registry changes, scheduled tasks or startup entries are used.
function Get-GuardName {
    param([string]$SelectedApp)
    return ('Local\CPUFix.' + $SelectedApp + '.' + $identity.User.Value)
}
function Enter-Mutex {
    param([Threading.Mutex]$Mutex, [int]$Milliseconds)
    try { return $Mutex.WaitOne($Milliseconds) }
    catch [Threading.AbandonedMutexException] { return $true }
}
function Test-GuardRunning {
    param([string]$SelectedApp)
    $guardName = Get-GuardName $SelectedApp
    $running = $null
    try {
        try { $running = [Threading.Mutex]::OpenExisting($guardName + '.Running') }
        catch [Threading.WaitHandleCannotBeOpenedException] { return $false }
        if (Enter-Mutex $running 0) {
            $running.ReleaseMutex()
            return $false
        }
        return $true
    } finally { if ($null -ne $running) { $running.Dispose() } }
}
function Stop-SessionGuard {
    param([string]$SelectedApp)
    $guardName = Get-GuardName $SelectedApp
    $running = $null
    $stop = $null
    try {
        try { $running = [Threading.Mutex]::OpenExisting($guardName + '.Running') }
        catch [Threading.WaitHandleCannotBeOpenedException] { return }
        if (Enter-Mutex $running 0) { $running.ReleaseMutex(); return }
        $stop = [Threading.EventWaitHandle]::OpenExisting($guardName + '.Stop')
        [void]$stop.Set()
        if (-not (Enter-Mutex $running 15000)) {
            throw ('后台守护尚未停止；没有恢复 CPU 4、5，请稍后重试 ' + $appInfo[$SelectedApp].RestoreScript + '。')
        }
        $running.ReleaseMutex()
    } finally {
        if ($null -ne $stop) { $stop.Dispose() }
        if ($null -ne $running) { $running.Dispose() }
    }
}
function Start-SessionGuard {
    param([string]$SelectedApp)
    $guardName = Get-GuardName $SelectedApp
    if (Test-GuardRunning $SelectedApp) {
        Write-Host '本次登录的后台守护已在运行，复用现有守护。' -ForegroundColor Green
        return
    }
    $ready = [Threading.EventWaitHandle]::new($false, [Threading.EventResetMode]::ManualReset, $guardName + '.Ready')
    $worker = $null
    $started = $false
    try {
        [void]$ready.Reset()
        $workerArgs = @('-NoProfile','-ExecutionPolicy','Bypass','-File',('"{0}"' -f $PSCommandPath),
            '-App',$SelectedApp,'-Mode','Avoid','-GuardWorker','-NoPause')
        $worker = Start-Process -FilePath (Join-Path $PSHOME 'powershell.exe') -ArgumentList $workerArgs -WindowStyle Hidden -PassThru
        $deadline = [DateTime]::UtcNow.AddSeconds(20)
        while ([DateTime]::UtcNow -lt $deadline) {
            if ($ready.WaitOne(200)) {
                if (-not $worker.HasExited -and (Test-GuardRunning $SelectedApp)) { $started = $true; break }
            }
            if ($worker.HasExited) { throw ('守护启动失败，退出码：' + $worker.ExitCode) }
        }
        if (-not $started) { throw '守护启动超时。' }
        Write-Host ('本次登录的后台守护已启动，PID：' + $worker.Id) -ForegroundColor Green
    } finally {
        # Only terminate the specific child we just launched if its startup failed.
        if ($null -ne $worker) {
            if (-not $started -and -not $worker.HasExited) { $worker.Kill(); [void]$worker.WaitForExit(5000) }
            $worker.Dispose()
        }
        $ready.Dispose()
    }
}
function Invoke-SessionGuard {
    param([string]$SelectedApp)
    $guardName = Get-GuardName $SelectedApp
    $running = [Threading.Mutex]::new($false, $guardName + '.Running')
    $ownsMutex = $false
    $stop = $null
    $ready = $null
    $source = 'CPUFix.' + $SelectedApp + '.ProcessStart.' + $PID
    $subscribed = $false
    $wasReady = $false
    $failure = $null
    try {
        $ownsMutex = Enter-Mutex $running 0
        if (-not $ownsMutex) { return }
        $stop = [Threading.EventWaitHandle]::new($false, [Threading.EventResetMode]::ManualReset, $guardName + '.Stop')
        [void]$stop.Reset()
        $ready = [Threading.EventWaitHandle]::new($false, [Threading.EventResetMode]::ManualReset, $guardName + '.Ready')
        # Process-start notifications reduce the delay. A periodic scan also covers missed events.
        $conditions = @($appInfo[$SelectedApp].Images | ForEach-Object { "ProcessName='$_'" })
        $query = 'SELECT * FROM Win32_ProcessStartTrace WHERE ' + ($conditions -join ' OR ')
        Register-WmiEvent -Query $query -SourceIdentifier $source -ErrorAction Stop | Out-Null
        $subscribed = $true
        $consecutiveErrors = 0
        while (-not $stop.WaitOne(0)) {
            $scan = Invoke-AppAffinity $SelectedApp 'Avoid' $false
            if ($scan.Failures.Count -gt 0) {
                $consecutiveErrors++
                if (-not $wasReady -or $consecutiveErrors -ge 5) { throw ($scan.Failures -join [Environment]::NewLine) }
            } else {
                $consecutiveErrors = 0
                if (-not $wasReady) { $wasReady = $true; [void]$ready.Set() }
            }
            # Empty results are normal: the app need not be open when the guard starts.
            if ($stop.WaitOne(0)) { break }
            Wait-Event -SourceIdentifier $source -Timeout 1 | Out-Null
            Get-Event -SourceIdentifier $source -ErrorAction SilentlyContinue | Remove-Event -ErrorAction SilentlyContinue
        }
    } catch {
        $failure = $_.Exception.Message
    } finally {
        if ($subscribed) { Unregister-Event -SourceIdentifier $source -ErrorAction SilentlyContinue }
        Get-Event -SourceIdentifier $source -ErrorAction SilentlyContinue | Remove-Event -ErrorAction SilentlyContinue
        if ($null -ne $ready) { $ready.Dispose() }
        if ($null -ne $stop) { $stop.Dispose() }
        if ($ownsMutex) { $running.ReleaseMutex() }
        $running.Dispose()
    }
    if ($null -ne $failure) {
        if ($wasReady) {
            # Report an actual guard failure instead of silently claiming continued protection.
            Add-Type -AssemblyName System.Windows.Forms
            [void][Windows.Forms.MessageBox]::Show(
                ($appInfo[$SelectedApp].Label + ' 的 CPU 守护已停止。请重新运行 ' + $appInfo[$SelectedApp].AvoidScript +
                    "；新进程暂时不会自动避开 CPU 4、5。`r`n`r`n" + $failure),
                'CPUFix 守护停止', [Windows.Forms.MessageBoxButtons]::OK, [Windows.Forms.MessageBoxIcon]::Warning)
        }
        throw $failure
    }
}

if ($GuardWorker) {
    try { Invoke-SessionGuard $App; exit 0 } catch { exit 1 }
}

function Invoke-AppOperation {
    param([string]$SelectedApp)
    $control = $null
    $ownsControl = $false
    $info = $appInfo[$SelectedApp]
    try {
        if (-not $Preview) {
            # Serialize controls for this app, and stop its worker before restoring affinity.
            $control = [Threading.Mutex]::new($false, ((Get-GuardName $SelectedApp) + '.Control'))
            $ownsControl = Enter-Mutex $control 25000
            if (-not $ownsControl) { throw '另一个 CPUFix 操作尚未结束，请稍后重试。' }
            if ($Mode -eq 'Restore') { Stop-SessionGuard $SelectedApp }
        }
        Write-Host ('操作范围：' + $info.Label)
        $scan = Invoke-AppAffinity $SelectedApp $Mode ([bool]$Preview)
        $scan.Results | Format-Table -AutoSize
        if ($scan.Results.Count -eq 0) { Write-Host '当前未运行，这不影响启动或停止守护。' }
        if ($scan.Failures.Count -gt 0) {
            foreach ($problem in $scan.Failures) { Write-Host $problem -ForegroundColor Red }
            throw '部分进程未设置成功，请查看上面的详情。'
        }
        if ($Preview) {
            Write-Host ('仅预览，没有修改进程或启停守护。守护正在运行：' + (Test-GuardRunning $SelectedApp))
        }
        elseif ($Mode -eq 'Avoid') {
            if ($scan.Results.Count -gt 0) { Write-Host '当前匹配进程已避开 CPU 4、5，读取验证通过。' -ForegroundColor Green }
            Start-SessionGuard $SelectedApp
            Write-Host ('可以关闭此窗口；本次登录中，后续打开或重开 ' + $info.Label + ' 都会自动避开 CPU 4、5。')
            Write-Host ('运行 ' + $info.RestoreScript + ' 会停止该软件的守护并恢复使用；注销或关机后守护结束，下次需重新运行 ' + $info.AvoidScript + '。')
            Write-Host '进程启动后有短暂检测延迟，不能保证从第一条指令起避开。' -ForegroundColor Yellow
        } else {
            Write-Host '该软件的后台守护已停止；现有匹配进程已允许使用 CPU 4、5，后续不会再自动避开。' -ForegroundColor Green
        }
    } finally {
        if ($ownsControl) { $control.ReleaseMutex() }
        if ($null -ne $control) { $control.Dispose() }
    }
}

$exitCode = 0
# All is a convenience for these three apps only, never every system process.
$selectedApps = if ($App -eq 'All') { @('VSCode','Chrome','ToDesk') } else { @($App) }
foreach ($selectedApp in $selectedApps) {
    try { Invoke-AppOperation $selectedApp }
    catch {
        Write-Host ($selectedApp + ' 未全部完成：' + $_.Exception.Message) -ForegroundColor Red
        $exitCode = 1
    }
}
Write-Host '这是应用进程设置，不会停用整机核心或禁止硬件中断。'
if (-not $NoPause) { [void](Read-Host '按回车关闭') }
# This script never writes logs or changes boot, firmware or system-wide settings.
exit $exitCode
