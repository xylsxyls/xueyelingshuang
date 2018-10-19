
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
set SQLite_dlllib=lib
set SQLite_bit=%1
set SQLite_debugRelease=%3
set SQLite_allSame=%4
if "%4" == "same" (goto SQLite_callSame) else (goto SQLite_callSimple)
:SQLite_callSame
set SQLite_dlllib=%2
call "%CLOUD_REBUILD%" SQLite %SQLite_bit% %SQLite_dlllib% %SQLite_debugRelease% %SQLite_allSame%
goto SQLite_end
:SQLite_callSimple
call "%CLOUD_REBUILD%" SQLite %SQLite_bit% %SQLite_dlllib% %SQLite_debugRelease%
goto SQLite_end
:SQLite_end

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