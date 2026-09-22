param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('Custom','VSCode','Chrome','ToDesk','All')][string]$App,
    [Parameter(Mandatory=$true)]
    [ValidateSet('Avoid','Restore')][string]$Mode,
    [switch]$Preview,
    [switch]$NoPause,
    [switch]$GuardWorker,
    [switch]$AutoStart
)
$ErrorActionPreference = 'Stop'
if ([IntPtr]::Size -ne 8) { throw 'Please use 64-bit PowerShell.' }
if ($GuardWorker) {
    if ($App -eq 'All' -or $Mode -ne 'Avoid' -or $Preview) { throw 'Invalid worker arguments.' }
    $NoPause = $true
}
if ($AutoStart) {
    if ($App -eq 'All' -or $Mode -ne 'Avoid' -or $Preview -or $GuardWorker) { throw 'Invalid startup arguments.' }
    $NoPause = $true
}
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
if (-not $Preview -and -not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    if ($AutoStart) { throw 'CPUFix login task must run with highest privileges.' }
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
    Custom = @{ Label='INI 自定义进程'; AvoidScript='01'; RestoreScript='02'; Images=@() }
    VSCode = @{ Label='VSCode / Codex'; AvoidScript='03'; RestoreScript='04'; Images=@('Code.exe','codex.exe','codex-code-mode-host.exe') }
    Chrome = @{ Label='Chrome'; AvoidScript='05'; RestoreScript='06'; Images=@('chrome.exe') }
    ToDesk = @{ Label='ToDesk'; AvoidScript='07'; RestoreScript='08'; Images=@('ToDesk.exe') }
}

$customStatePath = 'Software\CPUFix\CustomProcesses'
$script:customNames = @()
function ConvertTo-ProcessNames {
    param([string[]]$Values)
    $names = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($value in $Values) {
        if ([string]::IsNullOrWhiteSpace($value)) { continue }
        $name = $value.Trim()
        if ($name -notmatch '(?i)^[^\\/:*?"<>|,\[\]\x00-\x1F]+\.exe$' -or $name -eq '.exe') {
            throw ('无效的进程名：' + $name + '。请填写完整 EXE 文件名，不要填写路径或通配符。')
        }
        [void]$names.Add($name)
    }
    return @($names | Sort-Object)
}
function Read-CustomIni {
    $path = Join-Path $PSScriptRoot 'custom-processes.ini'
    if (-not [IO.File]::Exists($path)) { throw ('找不到配置文件：' + $path) }
    $section = ''; $value = $null
    foreach ($raw in [IO.File]::ReadAllLines($path)) {
        $line = $raw.Trim()
        if ($line.Length -eq 0 -or $line.StartsWith(';') -or $line.StartsWith('#')) { continue }
        if ($line -match '^\[([^\]]+)\]$') { $section = $Matches[1].Trim(); continue }
        if ($section -ne 'Processes') { continue }
        if ($line -notmatch '^Names\s*=(.*)$' -or $null -ne $value) {
            throw '配置格式有误：[Processes] 中应只有一行 Names=进程名，多个名称使用英文逗号分隔。'
        }
        $value = $Matches[1]
    }
    if ($null -eq $value) { throw '配置中缺少 [Processes] 下的 Names=。' }
    $names = @(ConvertTo-ProcessNames ($value -split ','))
    if ($names.Count -eq 0) { throw '进程名单为空；如果要取消避开，请运行 02。' }
    return $names
}
function Get-CustomState {
    $key = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey($customStatePath)
    if ($null -eq $key) { return [pscustomobject]@{Enabled=$false;Names=@();ManagedNames=@()} }
    try {
        return [pscustomobject]@{
            Enabled=($key.GetValue('Enabled', 0) -eq 1)
            Names=@(ConvertTo-ProcessNames ([string[]]$key.GetValue('Names', [string[]]@())))
            ManagedNames=@(ConvertTo-ProcessNames ([string[]]$key.GetValue('ManagedNames', [string[]]@())))
        }
    } finally { $key.Dispose() }
}
function Set-CustomState {
    param([bool]$Enabled, [string[]]$Names, [string[]]$ManagedNames)
    $key = [Microsoft.Win32.Registry]::CurrentUser.CreateSubKey($customStatePath)
    try {
        $key.SetValue('Names', [string[]]$Names, [Microsoft.Win32.RegistryValueKind]::MultiString)
        $key.SetValue('ManagedNames', [string[]]$ManagedNames, [Microsoft.Win32.RegistryValueKind]::MultiString)
        $key.SetValue('Enabled', [int]$Enabled, [Microsoft.Win32.RegistryValueKind]::DWord)
    } finally { $key.Dispose() }
}
function Clear-CustomState {
    $key = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey($customStatePath, $true)
    if ($null -eq $key) { return }
    try {
        foreach ($name in @('Enabled','Names','ManagedNames')) { $key.DeleteValue($name, $false) }
        $empty = $key.SubKeyCount -eq 0 -and $key.ValueCount -eq 0
    } finally { $key.Dispose() }
    if ($empty) { [Microsoft.Win32.Registry]::CurrentUser.DeleteSubKey($customStatePath, $false) }
}
function Get-BuiltinImageMap {
    param([string[]]$Apps)
    $allowed = [System.Collections.Generic.Dictionary[string,string]]::new([StringComparer]::OrdinalIgnoreCase)
    if ($Apps -contains 'VSCode') {
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
    if ($Apps -contains 'Chrome') {
        foreach ($path in @(
            'C:\Users\Administrator\AppData\Local\Google\Chrome\Bin\chrome.exe',
            'C:\Users\Administrator\AppData\Local\Google\Chrome\Application\chrome.exe',
            'C:\Program Files\Google\Chrome\Application\chrome.exe',
            'C:\Program Files (x86)\Google\Chrome\Application\chrome.exe'
        )) { $allowed[$path] = 'Chrome' }
    }
    if ($Apps -contains 'ToDesk') {
        $allowed['C:\Program Files\ToDesk\ToDesk.exe'] = 'ToDesk'
        $allowed['C:\Program Files (x86)\ToDesk\ToDesk.exe'] = 'ToDesk'
    }
    return ,$allowed
}
function Invoke-AppAffinity {
    param([string]$SelectedApp, [string]$SelectedMode, [bool]$PreviewOnly, [string[]]$NameOverride)
    # All retains its original meaning: the three built-in groups, not every process.
    $apps = if ($SelectedApp -eq 'All') { @('VSCode','Chrome','ToDesk') } else { @($SelectedApp) }
    $names = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $customTargets = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($selected in $apps) {
        $images = $appInfo[$selected].Images
        if ($selected -eq 'Custom') {
            $images = if ($PSBoundParameters.ContainsKey('NameOverride')) { @($NameOverride) } else { @($script:customNames) }
            foreach ($name in $images) { [void]$customTargets.Add($name) }
        }
        foreach ($name in $images) { [void]$names.Add([IO.Path]::GetFileNameWithoutExtension($name)) }
    }
    $mapApps = if ($SelectedMode -eq 'Restore') { @('VSCode','Chrome','ToDesk') } else { $apps }
    $allowed = Get-BuiltinImageMap $mapApps
    $otherBuiltins = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $otherCustom = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    if ($SelectedMode -eq 'Restore') {
        foreach ($other in @('VSCode','Chrome','ToDesk')) {
            if ($apps -contains $other) { continue }
            $task = Get-StartupTask $other
            if (($null -ne $task -and $task.Settings.Enabled) -or (Test-GuardRunning $other)) { [void]$otherBuiltins.Add($other) }
        }
        if ($apps -notcontains 'Custom') {
            $state = Get-CustomState
            if ($state.Enabled) { foreach ($name in $state.Names) { [void]$otherCustom.Add($name) } }
        }
    }
    $seen = [System.Collections.Generic.HashSet[string]]::new()
    $results = [System.Collections.Generic.List[object]]::new()
    $failures = [System.Collections.Generic.List[string]]::new()
    if ($names.Count -eq 0) { return [pscustomobject]@{Apps=$apps;Results=$results;Failures=$failures} }
    for ($pass = 1; $pass -le 2; $pass++) {
        foreach ($proc in @(Get-Process -Name ([string[]]@($names)) -ErrorAction SilentlyContinue)) {
            try {
                # Keep the handle open so this Process instance cannot target a reused PID.
                $heldHandle = $proc.Handle
                $imagePath = $proc.Path
                $imageName = $proc.ProcessName + '.exe'
                $builtin = if ($allowed.ContainsKey($imagePath)) { $allowed[$imagePath] } else { $null }
                $matched = if ($customTargets.Contains($imageName)) { 'Custom' } elseif ($null -ne $builtin -and $apps -contains $builtin) { $builtin } else { $null }
                if ($null -eq $matched) { continue }
                $started = $proc.StartTime.ToUniversalTime().ToString('o')
                if (-not $seen.Add(('{0}|{1}' -f $proc.Id,$started))) { continue }
                $before = $proc.ProcessorAffinity.ToInt64()
                $keptBy = ''
                if ($SelectedMode -eq 'Restore') {
                    if ($null -ne $builtin -and $otherBuiltins.Contains($builtin)) { $keptBy = $builtin }
                    if ($otherCustom.Contains($imageName)) { $keptBy = 'Custom' }
                }
                if ($SelectedMode -eq 'Avoid' -or $keptBy.Length -gt 0) { $desired = $before -band (-bnot [long]0x30) }
                else { $desired = $before -bor [long]0x30 }
                if ($desired -eq 0) { throw '设置后将没有可用 CPU，已拒绝。' }
                if (-not $PreviewOnly -and $before -ne $desired) { $proc.ProcessorAffinity = [IntPtr]$desired }
                $proc.Refresh()
                $after = $proc.ProcessorAffinity.ToInt64()
                if (-not $PreviewOnly -and $after -ne $desired) { throw '设置后的读取结果不一致。' }
                $results.Add([pscustomobject]@{
                    App=$matched; PID=$proc.Id; Process=$proc.ProcessName
                    Before=('0x{0:X}' -f $before); Target=('0x{0:X}' -f $desired)
                    After=('0x{0:X}' -f $after)
                    KeptBy=$keptBy
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

# One persistent login task per app. Its worker remains scoped to this login session.
# No passwords or extra runtime files are stored.
function Get-StartupTaskName {
    param([string]$SelectedApp)
    return ('CPUFix-' + $SelectedApp + '-AvoidCPU45')
}
function Get-StartupTaskDescription {
    param([string]$SelectedApp)
    return ('CPUFix.AffinityGuard.v1; User=' + $identity.User.Value + '; App=' + $SelectedApp)
}
function Get-StartupTask {
    param([string]$SelectedApp)
    $name = Get-StartupTaskName $SelectedApp
    $task = Get-ScheduledTask -TaskPath '\' -ErrorAction Stop | Where-Object { $_.TaskName -eq $name }
    if ($null -ne $task -and $task.Description -ne (Get-StartupTaskDescription $SelectedApp)) {
        throw ('同名计划任务不属于 CPUFix，未修改：' + $name)
    }
    return $task
}
function Set-StartupTask {
    param([string]$SelectedApp)
    # Check ownership before creating or updating an existing name.
    $null = Get-StartupTask $SelectedApp
    $arguments = '-NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "{0}" -App {1} -Mode Avoid -AutoStart -NoPause' -f $PSCommandPath,$SelectedApp
    $action = New-ScheduledTaskAction -Execute (Join-Path $PSHOME 'powershell.exe') -Argument $arguments -WorkingDirectory $PSScriptRoot
    $trigger = New-ScheduledTaskTrigger -AtLogOn -User $identity.Name
    $taskPrincipal = New-ScheduledTaskPrincipal -UserId $identity.User.Value -LogonType Interactive -RunLevel Highest
    $settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries `
        -StartWhenAvailable -MultipleInstances IgnoreNew -ExecutionTimeLimit ([TimeSpan]::Zero)
    Register-ScheduledTask -TaskName (Get-StartupTaskName $SelectedApp) -TaskPath '\' `
        -Action $action -Trigger $trigger -Principal $taskPrincipal -Settings $settings `
        -Description (Get-StartupTaskDescription $SelectedApp) -Force -ErrorAction Stop | Out-Null
    $saved = Get-StartupTask $SelectedApp
    if ($null -eq $saved -or -not $saved.Settings.Enabled -or $saved.Actions.Arguments -ne $arguments) {
        throw '登录自动启用设置未保存成功。'
    }
}
function Remove-StartupTask {
    param([string]$SelectedApp)
    $task = Get-StartupTask $SelectedApp
    if ($null -eq $task) { return }
    # Prevent future logins from starting a guard before stopping the current one.
    Disable-ScheduledTask -TaskName $task.TaskName -TaskPath $task.TaskPath -ErrorAction Stop | Out-Null
    Unregister-ScheduledTask -TaskName $task.TaskName -TaskPath $task.TaskPath -Confirm:$false -ErrorAction Stop
    if ($null -ne (Get-StartupTask $SelectedApp)) { throw '登录自动启用任务未能删除。' }
}

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
    if ($SelectedApp -eq 'Custom') {
        $state = Get-CustomState
        if (-not $state.Enabled -or $state.Names.Count -eq 0) { return }
        $script:customNames = @($state.Names)
        $appInfo.Custom.Images = @($state.Names)
    }
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
        $conditions = @($appInfo[$SelectedApp].Images | ForEach-Object { "ProcessName='" + $_.Replace("'", "\'") + "'" })
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
    $globalControl = $null
    $ownsGlobalControl = $false
    $removedCustom = @()
    $info = $appInfo[$SelectedApp]
    try {
        if (-not $Preview) {
            # Different groups can share a process; serialize policy changes across groups.
            $globalControl = [Threading.Mutex]::new($false, ('Local\CPUFix.Controls.' + $identity.User.Value))
            $ownsGlobalControl = Enter-Mutex $globalControl 60000
            if (-not $ownsGlobalControl) { throw '另一个 CPUFix 操作尚未结束，请稍后重试。' }
            # Serialize controls for this app, and stop its worker before restoring affinity.
            $control = [Threading.Mutex]::new($false, ((Get-GuardName $SelectedApp) + '.Control'))
            $ownsControl = Enter-Mutex $control 25000
            if (-not $ownsControl) { throw '另一个 CPUFix 操作尚未结束，请稍后重试。' }
            if ($AutoStart) {
                # A queued login invocation must not undo a later manual Restore.
                # This check and the guard start share the same control mutex as Restore.
                $startupTask = Get-StartupTask $SelectedApp
                if ($null -eq $startupTask -or -not $startupTask.Settings.Enabled) { return }
            }
            if ($Mode -eq 'Restore') {
                Remove-StartupTask $SelectedApp
                Stop-SessionGuard $SelectedApp
            }
        }
        if ($SelectedApp -eq 'Custom') {
            $state = Get-CustomState
            if ($Mode -eq 'Avoid') {
                if ($AutoStart -and -not $state.Enabled) { return }
                $newNames = if ($AutoStart) { @($state.Names) } else { @(Read-CustomIni) }
                if ($newNames.Count -eq 0) { throw '没有已保存的自定义进程名单，请运行 01。' }
                $script:customNames = @($newNames)
                if (-not $Preview) {
                    $previousNames = [System.Collections.Generic.HashSet[string]]::new([string[]]$state.Names, [StringComparer]::OrdinalIgnoreCase)
                    if (-not $previousNames.SetEquals([string[]]$newNames)) { Stop-SessionGuard $SelectedApp }
                    $history = @(ConvertTo-ProcessNames (@($state.ManagedNames) + @($state.Names) + @($newNames)))
                    # Save before changing affinity so a partial failure can still be undone.
                    Set-CustomState $true $newNames $history
                    $removedCustom = @($state.Names | Where-Object { $newNames -notcontains $_ })
                }
            } else {
                # Restore relies on saved history, even if the INI was edited or deleted.
                $script:customNames = @(ConvertTo-ProcessNames (@($state.ManagedNames) + @($state.Names)))
                if (-not $Preview -and $script:customNames.Count -gt 0) {
                    Set-CustomState $false $state.Names $script:customNames
                }
            }
            Write-Host ('进程名单：' + ($script:customNames -join ', '))
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
            $startupTask = Get-StartupTask $SelectedApp
            Write-Host ('已保存登录自动启用：' + ($null -ne $startupTask -and $startupTask.Settings.Enabled))
        }
        elseif ($Mode -eq 'Avoid') {
            if ($removedCustom.Count -gt 0) {
                $removedScan = Invoke-AppAffinity Custom Restore $false -NameOverride $removedCustom
                if ($removedScan.Failures.Count -gt 0) { throw ($removedScan.Failures -join [Environment]::NewLine) }
                Write-Host ('已撤销移出名单的进程规则：' + ($removedCustom -join ', '))
            }
            if ($scan.Results.Count -gt 0) { Write-Host '当前匹配进程已避开 CPU 4、5，读取验证通过。' -ForegroundColor Green }
            Start-SessionGuard $SelectedApp
            if (-not $AutoStart) { Set-StartupTask $SelectedApp }
            Write-Host ('可以关闭此窗口；后续打开或重开 ' + $info.Label + ' 都会自动避开 CPU 4、5，重启后登录也会自动继续。')
            Write-Host ('只有运行 ' + $info.RestoreScript + ' 才会取消登录自动启用、停止该软件的守护并恢复使用。重复运行不会增加任务或守护。')
            Write-Host '进程启动后有短暂检测延迟，不能保证从第一条指令起避开。' -ForegroundColor Yellow
        } else {
            if ($SelectedApp -eq 'Custom') { Clear-CustomState }
            Write-Host '已取消本组的登录自动启用并停止守护，恢复不受其他已启用规则管理的匹配进程。' -ForegroundColor Green
            if (@($scan.Results | Where-Object { $_.KeptBy.Length -gt 0 }).Count -gt 0) {
                Write-Host '部分进程仍由其他已启用的 CPUFix 规则管理，因此继续避开 CPU4/5。'
            }
        }
    } finally {
        if ($ownsControl) { $control.ReleaseMutex() }
        if ($null -ne $control) { $control.Dispose() }
        if ($ownsGlobalControl) { $globalControl.ReleaseMutex() }
        if ($null -ne $globalControl) { $globalControl.Dispose() }
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
# Persistence uses only the app-specific login tasks above; no logs or firmware changes.
exit $exitCode
