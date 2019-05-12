
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

xcopy /y /i /r /s "%xueyelingshuang%tools\gmp\include\gmp.h" "%xueyelingshuang%include\"

if "%1" == "32" (goto BigNumberBase_32) else (goto BigNumberBase_64)
:BigNumberBase_32
xcopy /y /i /r /s "%xueyelingshuang%tools\gmp\lib\gmp.lib" "%xueyelingshuang%lib\"
goto BigNumberBase_end
:BigNumberBase_64
xcopy /y /i /r /s "%xueyelingshuang%tools\gmp\gmp64.dll\gmp64.dll\libgmp.dll.lib" "%xueyelingshuang%lib\"
xcopy /y /i /r /s "%xueyelingshuang%tools\gmp\gmp64.dll\gmp64.dll\libgmp-10.dll" "%xueyelingshuang%lib\"
goto BigNumberBase_end
:BigNumberBase_end

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