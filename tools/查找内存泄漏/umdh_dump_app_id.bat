@echo off
set _NT_SYMBOL_PATH=D:\vs2012\Project3\Debug
echo _NT_SYMBOL_PATH=%_NT_SYMBOL_PATH%
echo make sure _NT_SYMBOL_PATH is right, if not, edit the bat
echo ----

:Start
echo input app name=(such as iexplore.exe)
set /p appname=
echo input process pid=
set /p appid=

gflags -i %appname% +ust

:Loop
echo.
echo 1: umdh.exe -p:%appid% -f:UmdhDump1.txt
echo 2: umdh.exe -p:%appid% -f:UmdhDump2.txt
echo 3: umdh.exe -d UmdhDump1.txt UmdhDump2.txt -f:UmdhDumpResult.txt
echo 4: exit
echo 5: restart
echo select: 
set /p select=
if %select%==1 (
umdh.exe -p:%appid% -f:UmdhDump1.txt
)
if %select%==2 (
umdh.exe -p:%appid% -f:UmdhDump2.txt
)
if %select%==3 (
umdh.exe -d UmdhDump1.txt UmdhDump2.txt -f:UmdhDumpResult.txt
)
if %select%==4 (
goto End
)
if %select%==5 (
goto Start
)
goto Loop

:End