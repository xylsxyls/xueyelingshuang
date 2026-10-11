$ErrorActionPreference = 'Stop'
$testRoot = Split-Path -Parent $PSScriptRoot
$testArguments = @($args)
try {
    $localFile = Join-Path $testRoot 'config.local.json'
    $settings = if (Test-Path -LiteralPath $localFile) { Get-Content -Encoding UTF8 -LiteralPath $localFile -Raw | ConvertFrom-Json } else { $null }
    $nodeCommand = Get-Command node.exe -ErrorAction SilentlyContinue
    $testNode = if ($settings -and $settings.nodePath) { $settings.nodePath } elseif ($nodeCommand) { $nodeCommand.Source } else { Join-Path ${env:ProgramFiles(x86)} 'Tencent\微信web开发者工具\node-18.exe' }
    if (-not (Test-Path -LiteralPath $testNode)) { throw '没有找到 Node 18。请安装 Node 18+ 或在 config.local.json 设置 nodePath。' }
    $version = & $testNode --version
    if ($version -notmatch '^v(\d+)\.' -or [int]$Matches[1] -lt 18) { throw '测试程序需要 Node 18 或更新版本。' }
    if ($testArguments.Count -eq 0) {
        Write-Host '1 按编号  2 功能测试  3 压力测试  4 全部测试  5 列出用例  6 设施自检  7 准备模拟器副本  8 仅服务端测试  9 官方语法编译'
        $choice = Read-Host '输入选项'
        switch ($choice) {
            '1' { $testArguments = @('--ids=' + (Read-Host '输入编号，如1,3,8-12')) }
            '2' { $testArguments = @('--instant') }
            '3' { $testArguments = @('--stress') }
            '4' { $testArguments = @('--all') }
            '5' { $testArguments = @('--list') }
            '6' { $testArguments = @('--selfcheck') }
            '7' { $testArguments = @('--prepare') }
            '8' { $testArguments = @('--backend-only', '--ids=34,36,44,51') }
            '9' { $testArguments = @('--compile') }
            default { throw '选项无效。' }
        }
    }
    if ($testArguments -contains '--compile') {
        if ($testArguments.Count -ne 1) { throw '语法编译不能和其他测试参数混用。' }
        & $testNode (Join-Path $testRoot 'scripts/compile.js')
        exit $LASTEXITCODE
    }
    if (-not (Test-Path -LiteralPath (Join-Path $testRoot 'node_modules/miniprogram-automator/package.json'))) {
        # npm 固定版本仅用于安装锁文件，不要求全局安装或修改 PATH。
        $installRoot = Join-Path $testRoot 'temp/setup'
        New-Item -ItemType Directory -Path $installRoot -Force | Out-Null
        $archive = Join-Path $installRoot 'npm-10.8.2.tgz'
        if (-not (Test-Path -LiteralPath $archive)) { Invoke-WebRequest -UseBasicParsing -Uri 'https://registry.npmjs.org/npm/-/npm-10.8.2.tgz' -OutFile $archive }
        & tar.exe -xf $archive -C $installRoot
        if ($LASTEXITCODE -ne 0) { throw 'npm 压缩包解压失败。' }
        Push-Location $testRoot
        try {
            $npmCacheDirectory = Join-Path $installRoot 'npm-cache'
            & $testNode (Join-Path $installRoot 'package/bin/npm-cli.js') ci --ignore-scripts --registry=https://registry.npmjs.org "--cache=$npmCacheDirectory" --no-audit --no-fund
            if ($LASTEXITCODE -ne 0) { throw '测试依赖安装失败，检查网络后重新运行。' }
        } finally { Pop-Location }
    }
    if ($testArguments -contains '--selfcheck') {
        if ($testArguments.Count -ne 1) { throw '设施自检不能和产品用例参数混用。' }
        & $testNode (Join-Path $testRoot 'selfcheck/run.js')
    } else { & $testNode (Join-Path $testRoot 'index.js') @testArguments }
    exit $LASTEXITCODE
} catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}