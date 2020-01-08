
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

xcopy /y /i /r /s "%xueyelingshuang%tools\civetweb\include\*" "%xueyelingshuang%include\civetweb\"

if "%1" == "32" (goto HttpService_32) else (goto HttpService_64)
:HttpService_32
if "%3" == "release" (goto HttpService_release) else (goto HttpService_debug)
:HttpService_release
xcopy /y /i /r /s "%xueyelingshuang%tools\civetweb\win32\release\*.lib" "%xueyelingshuang%lib\"
goto HttpService_end
:HttpService_64
if "%3" == "release" (goto HttpService_64_release) else (goto HttpService_64_debug)
:HttpService_64_debug
xcopy /y /i /r /s "%xueyelingshuang%tools\civetweb\win64\debug\*.lib" "%xueyelingshuang%lib\"
goto HttpService_end
:HttpService_64_release
xcopy /y /i /r /s "%xueyelingshuang%tools\civetweb\win64\release\*.lib" "%xueyelingshuang%lib\"
goto HttpService_end
:HttpService_end
goto HttpService_debug_release_end
:HttpService_debug
xcopy /y /i /r /s "%xueyelingshuang%tools\civetweb\win32\debug\*.lib" "%xueyelingshuang%lib\"
goto HttpService_debug_release_end
:HttpService_debug_release_end