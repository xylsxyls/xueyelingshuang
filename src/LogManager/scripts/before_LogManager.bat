
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
set CGetPath_dlllib=lib
set CGetPath_bit=%1
set CGetPath_debugRelease=%3
set CGetPath_allSame=%4
if "%4" == "same" (goto CGetPath_callSame) else (goto CGetPath_callSimple)
:CGetPath_callSame
set CGetPath_dlllib=%2
call "%CLOUD_REBUILD%" CGetPath %CGetPath_bit% %CGetPath_dlllib% %CGetPath_debugRelease% %CGetPath_allSame%
goto CGetPath_end
:CGetPath_callSimple
call "%CLOUD_REBUILD%" CGetPath %CGetPath_bit% %CGetPath_dlllib% %CGetPath_debugRelease%
goto CGetPath_end
:CGetPath_end

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

::--------------------------------------------------------------------
set IntDateTime_dlllib=lib
set IntDateTime_bit=%1
set IntDateTime_debugRelease=%3
set IntDateTime_allSame=%4
if "%4" == "same" (goto IntDateTime_callSame) else (goto IntDateTime_callSimple)
:IntDateTime_callSame
set IntDateTime_dlllib=%2
call "%CLOUD_REBUILD%" IntDateTime %IntDateTime_bit% %IntDateTime_dlllib% %IntDateTime_debugRelease% %IntDateTime_allSame%
goto IntDateTime_end
:IntDateTime_callSimple
call "%CLOUD_REBUILD%" IntDateTime %IntDateTime_bit% %IntDateTime_dlllib% %IntDateTime_debugRelease%
goto IntDateTime_end
:IntDateTime_end