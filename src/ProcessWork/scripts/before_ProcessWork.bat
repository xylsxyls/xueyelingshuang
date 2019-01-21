
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
set SharedMemory_dlllib=lib
set SharedMemory_bit=%1
set SharedMemory_debugRelease=%3
set SharedMemory_allSame=%4
if "%4" == "same" (goto SharedMemory_callSame) else (goto SharedMemory_callSimple)
:SharedMemory_callSame
set SharedMemory_dlllib=%2
call "%CLOUD_REBUILD%" SharedMemory %SharedMemory_bit% %SharedMemory_dlllib% %SharedMemory_debugRelease% %SharedMemory_allSame%
goto SharedMemory_end
:SharedMemory_callSimple
call "%CLOUD_REBUILD%" SharedMemory %SharedMemory_bit% %SharedMemory_dlllib% %SharedMemory_debugRelease%
goto SharedMemory_end
:SharedMemory_end

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
set LockFreeQueue_dlllib=lib
set LockFreeQueue_bit=%1
set LockFreeQueue_debugRelease=%3
set LockFreeQueue_allSame=%4
if "%4" == "same" (goto LockFreeQueue_callSame) else (goto LockFreeQueue_callSimple)
:LockFreeQueue_callSame
set LockFreeQueue_dlllib=%2
call "%CLOUD_REBUILD%" LockFreeQueue %LockFreeQueue_bit% %LockFreeQueue_dlllib% %LockFreeQueue_debugRelease% %LockFreeQueue_allSame%
goto LockFreeQueue_end
:LockFreeQueue_callSimple
call "%CLOUD_REBUILD%" LockFreeQueue %LockFreeQueue_bit% %LockFreeQueue_dlllib% %LockFreeQueue_debugRelease%
goto LockFreeQueue_end
:LockFreeQueue_end

::--------------------------------------------------------------------
set CorrespondParam_dlllib=lib
set CorrespondParam_bit=%1
set CorrespondParam_debugRelease=%3
set CorrespondParam_allSame=%4
if "%4" == "same" (goto CorrespondParam_callSame) else (goto CorrespondParam_callSimple)
:CorrespondParam_callSame
set CorrespondParam_dlllib=%2
call "%CLOUD_REBUILD%" CorrespondParam %CorrespondParam_bit% %CorrespondParam_dlllib% %CorrespondParam_debugRelease% %CorrespondParam_allSame%
goto CorrespondParam_end
:CorrespondParam_callSimple
call "%CLOUD_REBUILD%" CorrespondParam %CorrespondParam_bit% %CorrespondParam_dlllib% %CorrespondParam_debugRelease%
goto CorrespondParam_end
:CorrespondParam_end

::--------------------------------------------------------------------
set LogManager_dlllib=lib
set LogManager_bit=%1
set LogManager_debugRelease=%3
set LogManager_allSame=%4
if "%4" == "same" (goto LogManager_callSame) else (goto LogManager_callSimple)
:LogManager_callSame
set LogManager_dlllib=%2
call "%CLOUD_REBUILD%" LogManager %LogManager_bit% %LogManager_dlllib% %LogManager_debugRelease% %LogManager_allSame%
goto LogManager_end
:LogManager_callSimple
call "%CLOUD_REBUILD%" LogManager %LogManager_bit% %LogManager_dlllib% %LogManager_debugRelease%
goto LogManager_end
:LogManager_end