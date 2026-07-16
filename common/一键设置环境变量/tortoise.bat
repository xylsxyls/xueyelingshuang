@echo off
chcp 65001 >nul
:: 自动提权管理员
fltmc filters >nul 2>&1 || (
    powershell -Command "Start-Process cmd -ArgumentList '/c ""%~f0""' -Verb RunAs"
    exit /b
)

setlocal enabledelayedexpansion
set "basePath=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\ShellIconOverlayIdentifiers"

:: ======================
:: 配对数组，格式：旧项名|新项名
:: 您可以随意增删行，只要索引连续即可（0,1,2,...）
:: 旧项名请写注册表中实际的完整名称（包括前导空格）
:: ======================
set "pair[0]=  Tortoise1Normal|Tortoise1Normal"
set "pair[1]=  Tortoise2Modified|Tortoise2Modified"
set "pair[2]=  Tortoise3Conflict|Tortoise3Conflict"
set "pair[3]=  Tortoise4Locked|Tortoise4Locked"
set "pair[4]=  Tortoise5ReadOnly|Tortoise5ReadOnly"
set "pair[5]=  Tortoise6Deleted|Tortoise6Deleted"
set "pair[6]=  Tortoise7Added|Tortoise7Added"
set "pair[7]=  Tortoise8Ignored|Tortoise8Ignored"
set "pair[8]=  Tortoise9Unversioned|Tortoise9Unversioned"
:: 您可以继续添加，例如：
:: set "pair[9]=       360UDiskGuard Icon Overlay|360UDiskGuard Icon Overlay"

echo ==============================
echo 批量重命名 ShellIconOverlayIdentifiers 项
echo ==============================
echo.

set idx=0
:loop
if not defined pair[%idx%] goto endAll
set "line=!pair[%idx%]!"
for /f "tokens=1,2 delims=|" %%a in ("!line!") do (
    set "oldKey=%%a"
    set "newKey=%%b"
)
echo 【%idx%】旧项:"!oldKey!"  →  新项:"!newKey!"

:: ---------- 跳过不存在的旧项 ----------
reg query "%basePath%\!oldKey!" >nul 2>&1
if errorlevel 1 (
    echo     旧项不存在，跳过
    echo.
    set /a idx+=1
    goto loop
)
:: --------------------------------------

echo     发现旧项，执行重命名...
reg copy "%basePath%\!oldKey!" "%basePath%\!newKey!" /s /f >nul
if errorlevel 1 (
    echo     复制失败，请检查权限或目标是否存在
) else (
    reg delete "%basePath%\!oldKey!" /f >nul
    echo     重命名完成
)
echo.
set /a idx+=1
goto loop

:endAll
echo ==============================
echo 全部处理完毕，重启资源管理器
echo ==============================
taskkill /f /im explorer.exe && start explorer.exe
pause