
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
set CStringManager_dlllib=lib
set CStringManager_bit=%1
set CStringManager_debugRelease=%3
set CStringManager_allSame=%4
if "%4" == "same" (goto CStringManager_callSame) else (goto CStringManager_callSimple)
:CStringManager_callSame
set CStringManager_dlllib=%2
call "%CLOUD_REBUILD%" CStringManager %CStringManager_bit% %CStringManager_dlllib% %CStringManager_debugRelease% %CStringManager_allSame%
goto CStringManager_end
:CStringManager_callSimple
call "%CLOUD_REBUILD%" CStringManager %CStringManager_bit% %CStringManager_dlllib% %CStringManager_debugRelease%
goto CStringManager_end
:CStringManager_end