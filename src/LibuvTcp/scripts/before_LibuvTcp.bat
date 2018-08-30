
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

::--------------------------------------------------------------------
set ReadWriteMutex_dlllib=lib
set ReadWriteMutex_bit=%1
set ReadWriteMutex_debugRelease=%3
set ReadWriteMutex_allSame=%4
if "%4" == "same" (goto ReadWriteMutex_callSame) else (goto ReadWriteMutex_callSimple)
:ReadWriteMutex_callSame
set ReadWriteMutex_dlllib=%2
call "%CLOUD_REBUILD%" ReadWriteMutex %ReadWriteMutex_bit% %ReadWriteMutex_dlllib% %ReadWriteMutex_debugRelease% %ReadWriteMutex_allSame%
goto ReadWriteMutex_end
:ReadWriteMutex_callSimple
call "%CLOUD_REBUILD%" ReadWriteMutex %ReadWriteMutex_bit% %ReadWriteMutex_dlllib% %ReadWriteMutex_debugRelease%
goto ReadWriteMutex_end
:ReadWriteMutex_end

set bat=%~dp0
set xueyelingshuang=%bat%..\..\..\

xcopy /y /i /r /s "%xueyelingshuang%tools\libuv\libuv\*" "%xueyelingshuang%include\libuv\"
xcopy /y /i /r /s "%xueyelingshuang%tools\libuv\libuvd.lib" "%xueyelingshuang%lib\"
xcopy /y /i /r /s "%xueyelingshuang%tools\libuv\libuv.lib" "%xueyelingshuang%lib\"