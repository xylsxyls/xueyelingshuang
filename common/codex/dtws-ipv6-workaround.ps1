#Requires -Version 5.1
#Requires -RunAsAdministrator

[CmdletBinding(SupportsShouldProcess)]
param(
    [ValidateSet('Apply', 'Status', 'Remove')]
    [string]$Action = 'Apply',

    [string]$GamePath
)

$ErrorActionPreference = 'Stop'
$ruleName = 'DTWS-IPv6-Pharos-19037-Test'
$remoteAddress = '2407:ad80:1013:e001::e001:3d8'
$remotePort = '19037'

function Find-GameExecutable {
    param([string[]]$Locations)

    foreach ($location in $Locations) {
        if ([string]::IsNullOrWhiteSpace($location)) { continue }

        $item = Get-Item -LiteralPath $location -ErrorAction SilentlyContinue
        if (-not $item) { continue }

        if (-not $item.PSIsContainer) {
            if ($item.Name -eq 'GacRunner.dll') { return $item.FullName }
            continue
        }

        foreach ($relativePath in @('GacRunner.dll', 'bin\Release\GacRunner.dll', 'dtws\bin\Release\GacRunner.dll')) {
            $candidate = Join-Path $item.FullName $relativePath
            if (Test-Path -LiteralPath $candidate -PathType Leaf) {
                return (Get-Item -LiteralPath $candidate).FullName
            }
        }
    }
}

$rule = Get-NetFirewallRule -Name $ruleName -PolicyStore PersistentStore -ErrorAction SilentlyContinue

if ($Action -eq 'Remove') {
    if (-not $rule) {
        Write-Output 'DTWS IPv6 workaround is already absent.'
    } elseif ($PSCmdlet.ShouldProcess($ruleName, 'Remove DTWS firewall rule')) {
        $rule | Remove-NetFirewallRule
        Write-Output 'DTWS IPv6 workaround removed.'
    }
    return
}

if ($Action -eq 'Apply') {
    if ($PSBoundParameters.ContainsKey('GamePath')) {
        $gameExe = Find-GameExecutable -Locations $GamePath
    } else {
        $runningPaths = @(Get-Process -Name 'GacRunner.dll' -ErrorAction SilentlyContinue |
            ForEach-Object { $_.Path } | Where-Object { $_ } | Select-Object -Unique)
        if ($runningPaths.Count -gt 1) {
            throw 'Multiple game installations are running. Specify the desired installation with -GamePath.'
        }

        $gameExe = Find-GameExecutable -Locations $runningPaths
        if (-not $gameExe -and $env:LOCALAPPDATA) {
            $configPath = Join-Path $env:LOCALAPPDATA 'DTWS\local_path.json'
            if (Test-Path -LiteralPath $configPath -PathType Leaf) {
                try {
                    $config = Get-Content -LiteralPath $configPath -Raw -Encoding UTF8 | ConvertFrom-Json
                    $gameExe = Find-GameExecutable -Locations $config.dtws
                } catch {
                    Write-Verbose "Could not read the launcher installation path: $($_.Exception.Message)"
                }
            }
        }
        if (-not $gameExe) {
            $defaultLocations = foreach ($folder in @(${env:ProgramFiles(x86)}, $env:ProgramFiles)) {
                if ($folder) { Join-Path $folder 'dtws' }
            }
            $gameExe = Find-GameExecutable -Locations $defaultLocations
        }
    }

    if (-not $gameExe) {
        throw 'GacRunner.dll was not found. Start DTWS or specify the installation folder or executable with -GamePath.'
    }

    $settings = @{
        Name          = $ruleName
        PolicyStore   = 'PersistentStore'
        Enabled       = 'True'
        Profile       = 'Any'
        Direction     = 'Outbound'
        Action        = 'Block'
        Program       = $gameExe
        Protocol      = 'TCP'
        RemoteAddress = $remoteAddress
        RemotePort    = $remotePort
    }

    $ruleMatches = $false
    if ($rule) {
        $app = $rule | Get-NetFirewallApplicationFilter
        $address = $rule | Get-NetFirewallAddressFilter
        $port = $rule | Get-NetFirewallPortFilter
        $ruleMatches = $rule.Enabled -eq 'True' -and $rule.Profile -eq 'Any' -and
            $rule.Direction -eq 'Outbound' -and $rule.Action -eq 'Block' -and
            $app.Program -eq $gameExe -and ($address.RemoteAddress -join ',') -eq $remoteAddress -and
            $port.Protocol -in @('TCP', 6) -and ($port.RemotePort -join ',') -eq $remotePort
    }

    if (-not $ruleMatches) {
        if (-not $PSCmdlet.ShouldProcess($gameExe, "Block DTWS TCP connections to [$remoteAddress]:$remotePort")) { return }
        if ($rule) {
            Set-NetFirewallRule @settings
        } else {
            $settings.DisplayName = 'DTWS IPv6 Pharos 19037 test'
            $settings.Description = 'Workaround for one Datang Wushuang cross-server acceleration endpoint.'
            New-NetFirewallRule @settings | Out-Null
        }
        $rule = Get-NetFirewallRule -Name $ruleName -PolicyStore PersistentStore
    }
}

if (-not $rule) {
    Write-Output 'DTWS IPv6 workaround is absent.'
    return
}

$effective = Get-NetFirewallRule -Name $ruleName -PolicyStore ActiveStore -ErrorAction SilentlyContinue
$app = $rule | Get-NetFirewallApplicationFilter
$address = $rule | Get-NetFirewallAddressFilter
$port = $rule | Get-NetFirewallPortFilter
[pscustomobject]@{
    Rule        = $rule.Name
    Enabled     = $rule.Enabled
    Enforced    = [bool]($effective.EnforcementStatus -contains 'Enforced')
    Program     = $app.Program
    Direction   = $rule.Direction
    Action      = $rule.Action
    Protocol    = $port.Protocol
    Remote      = "[$($address.RemoteAddress -join ',')]:$($port.RemotePort -join ',')"
} | Format-List

if ($effective.EnforcementStatus -notcontains 'Enforced') {
    Write-Warning 'The rule is saved, but Windows does not report it as enforced. Check active firewall policies.'
}
