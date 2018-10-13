
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