
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
set DmSoft_dlllib=lib
set DmSoft_bit=%1
set DmSoft_debugRelease=%3
set DmSoft_allSame=%4
if "%4" == "same" (goto DmSoft_callSame) else (goto DmSoft_callSimple)
:DmSoft_callSame
set DmSoft_dlllib=%2
call "%CLOUD_REBUILD%" DmSoft %DmSoft_bit% %DmSoft_dlllib% %DmSoft_debugRelease% %DmSoft_allSame%
goto DmSoft_end
:DmSoft_callSimple
call "%CLOUD_REBUILD%" DmSoft %DmSoft_bit% %DmSoft_dlllib% %DmSoft_debugRelease%
goto DmSoft_end
:DmSoft_end

::--------------------------------------------------------------------
set Rect_dlllib=lib
set Rect_bit=%1
set Rect_debugRelease=%3
set Rect_allSame=%4
if "%4" == "same" (goto Rect_callSame) else (goto Rect_callSimple)
:Rect_callSame
set Rect_dlllib=%2
call "%CLOUD_REBUILD%" Rect %Rect_bit% %Rect_dlllib% %Rect_debugRelease% %Rect_allSame%
goto Rect_end
:Rect_callSimple
call "%CLOUD_REBUILD%" Rect %Rect_bit% %Rect_dlllib% %Rect_debugRelease%
goto Rect_end
:Rect_end

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