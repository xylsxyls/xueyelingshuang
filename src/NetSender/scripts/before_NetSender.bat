
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
set ProcessClient_dlllib=lib
set ProcessClient_bit=%1
set ProcessClient_debugRelease=%3
set ProcessClient_allSame=%4
if "%4" == "same" (goto ProcessClient_callSame) else (goto ProcessClient_callSimple)
:ProcessClient_callSame
set ProcessClient_dlllib=%2
call "%CLOUD_REBUILD%" ProcessClient %ProcessClient_bit% %ProcessClient_dlllib% %ProcessClient_debugRelease% %ProcessClient_allSame%
goto ProcessClient_end
:ProcessClient_callSimple
call "%CLOUD_REBUILD%" ProcessClient %ProcessClient_bit% %ProcessClient_dlllib% %ProcessClient_debugRelease%
goto ProcessClient_end
:ProcessClient_end