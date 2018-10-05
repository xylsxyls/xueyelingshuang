
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