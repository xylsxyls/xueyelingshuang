
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
set core_dlllib=lib
set core_bit=%1
set core_debugRelease=%3
set core_allSame=%4
if "%4" == "same" (goto core_callSame) else (goto core_callSimple)
:core_callSame
set core_dlllib=%2
call "%CLOUD_REBUILD%" core %core_bit% %core_dlllib% %core_debugRelease% %core_allSame%
goto core_end
:core_callSimple
call "%CLOUD_REBUILD%" core %core_bit% %core_dlllib% %core_debugRelease%
goto core_end
:core_end