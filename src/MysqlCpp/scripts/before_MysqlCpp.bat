
::--------------------------------------------------------------------
set DllRelyTest_dlllib=lib
set DllRelyTest_bit=%1
set DllRelyTest_debugRelease=%3
set DllRelyTest_allSame=%4
if "%4" == "same" (goto DllRelyTest_callSame) else (goto DllRelyTest_callSimple)
:DllRelyTest_callSame
set DllRelyTest_dlllib=%2
call "%CLOUD_REBUILD%" DllRelyTest %DllRelyTest_bit% %DllRelyTest_dlllib% %DllRelyTest_debugRelease% %DllRelyTest_allSame%
goto DllRelyTest_end
:DllRelyTest_callSimple
call "%CLOUD_REBUILD%" DllRelyTest %DllRelyTest_bit% %DllRelyTest_dlllib% %DllRelyTest_debugRelease%
goto DllRelyTest_end
:DllRelyTest_end

::--------------------------------------------------------------------
set bat=%~dp0
set xueyelingshuang=%bat%..\..\..\

call "%xueyelingshuang%src\mysqlcpp\public.bat" %1 %3

::--------------------------------------------------------------------
set SqlString_dlllib=lib
set SqlString_bit=%1
set SqlString_debugRelease=%3
set SqlString_allSame=same
if "%4" == "same" (goto SqlString_callSame) else (goto SqlString_callSimple)
:SqlString_callSame
set SqlString_dlllib=lib
call "%CLOUD_REBUILD%" SqlString %SqlString_bit% %SqlString_dlllib% %SqlString_debugRelease% %SqlString_allSame%
goto SqlString_end
:SqlString_callSimple
call "%CLOUD_REBUILD%" SqlString %SqlString_bit% %SqlString_dlllib% %SqlString_debugRelease%
goto SqlString_end
:SqlString_end

::--------------------------------------------------------------------
set CSystem_dlllib=lib
set CSystem_bit=%1
set CSystem_debugRelease=%3
set CSystem_allSame=%4
if "%4" == "same" (goto CSystem_callSame) else (goto CSystem_callSimple)
:CSystem_callSame
set CSystem_dlllib=%2
call "%CLOUD_REBUILD%" CSystem %CSystem_bit% %CSystem_dlllib% %CSystem_debugRelease% %CSystem_allSame%
goto CSystem_end
:CSystem_callSimple
call "%CLOUD_REBUILD%" CSystem %CSystem_bit% %CSystem_dlllib% %CSystem_debugRelease%
goto CSystem_end
:CSystem_end