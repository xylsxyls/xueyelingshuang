
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
set Semaphore_dlllib=lib
set Semaphore_bit=%1
set Semaphore_debugRelease=%3
set Semaphore_allSame=%4
if "%4" == "same" (goto Semaphore_callSame) else (goto Semaphore_callSimple)
:Semaphore_callSame
set Semaphore_dlllib=%2
call "%CLOUD_REBUILD%" Semaphore %Semaphore_bit% %Semaphore_dlllib% %Semaphore_debugRelease% %Semaphore_allSame%
goto Semaphore_end
:Semaphore_callSimple
call "%CLOUD_REBUILD%" Semaphore %Semaphore_bit% %Semaphore_dlllib% %Semaphore_debugRelease%
goto Semaphore_end
:Semaphore_end