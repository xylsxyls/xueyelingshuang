
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
set Point_dlllib=lib
set Point_bit=%1
set Point_debugRelease=%3
set Point_allSame=%4
if "%4" == "same" (goto Point_callSame) else (goto Point_callSimple)
:Point_callSame
set Point_dlllib=%2
call "%CLOUD_REBUILD%" Point %Point_bit% %Point_dlllib% %Point_debugRelease% %Point_allSame%
goto Point_end
:Point_callSimple
call "%CLOUD_REBUILD%" Point %Point_bit% %Point_dlllib% %Point_debugRelease%
goto Point_end
:Point_end