
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
set NetWork_dlllib=lib
set NetWork_bit=%1
set NetWork_debugRelease=%3
set NetWork_allSame=%4
if "%4" == "same" (goto NetWork_callSame) else (goto NetWork_callSimple)
:NetWork_callSame
set NetWork_dlllib=%2
call "%CLOUD_REBUILD%" NetWork %NetWork_bit% %NetWork_dlllib% %NetWork_debugRelease% %NetWork_allSame%
goto NetWork_end
:NetWork_callSimple
call "%CLOUD_REBUILD%" NetWork %NetWork_bit% %NetWork_dlllib% %NetWork_debugRelease%
goto NetWork_end
:NetWork_end