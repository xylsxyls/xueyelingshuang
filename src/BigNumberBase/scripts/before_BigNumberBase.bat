
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
set bat=%~dp0
set xueyelingshuang=%bat%..\..\..\

xcopy /y /i /r /s "%xueyelingshuang%tools\gmp\gmp32.static\gmp-static-p4\gmp.h" "%xueyelingshuang%include\"
xcopy /y /i /r /s "%xueyelingshuang%tools\gmp\gmp32.static\gmp-static-p4\libgcc.a" "%xueyelingshuang%lib\"
xcopy /y /i /r /s "%xueyelingshuang%tools\gmp\gmp32.static\gmp-static-p4\libgmp.a" "%xueyelingshuang%lib\"
xcopy /y /i /r /s "%xueyelingshuang%tools\gmp\gmp32.static\gmp-static-p4\libmingwex.a" "%xueyelingshuang%lib\"
xcopy /y /i /r /s "%xueyelingshuang%tools\gmp\gmp32.static\gmp-static-p4\libmsvcrt.a" "%xueyelingshuang%lib\"

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