
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
set BigNumber_dlllib=lib
set BigNumber_bit=%1
set BigNumber_debugRelease=%3
set BigNumber_allSame=%4
if "%4" == "same" (goto BigNumber_callSame) else (goto BigNumber_callSimple)
:BigNumber_callSame
set BigNumber_dlllib=%2
call "%CLOUD_REBUILD%" BigNumber %BigNumber_bit% %BigNumber_dlllib% %BigNumber_debugRelease% %BigNumber_allSame%
goto BigNumber_end
:BigNumber_callSimple
call "%CLOUD_REBUILD%" BigNumber %BigNumber_bit% %BigNumber_dlllib% %BigNumber_debugRelease%
goto BigNumber_end
:BigNumber_end