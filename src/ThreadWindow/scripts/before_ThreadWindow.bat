
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
set CTaskThreadManager_dlllib=lib
set CTaskThreadManager_bit=%1
set CTaskThreadManager_debugRelease=%3
set CTaskThreadManager_allSame=%4
if "%4" == "same" (goto CTaskThreadManager_callSame) else (goto CTaskThreadManager_callSimple)
:CTaskThreadManager_callSame
set CTaskThreadManager_dlllib=%2
call "%CLOUD_REBUILD%" CTaskThreadManager %CTaskThreadManager_bit% %CTaskThreadManager_dlllib% %CTaskThreadManager_debugRelease% %CTaskThreadManager_allSame%
goto CTaskThreadManager_end
:CTaskThreadManager_callSimple
call "%CLOUD_REBUILD%" CTaskThreadManager %CTaskThreadManager_bit% %CTaskThreadManager_dlllib% %CTaskThreadManager_debugRelease%
goto CTaskThreadManager_end
:CTaskThreadManager_end