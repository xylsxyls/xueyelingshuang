
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
set BigNumberBase_dlllib=lib
set BigNumberBase_bit=%1
set BigNumberBase_debugRelease=%3
set BigNumberBase_allSame=%4
if "%4" == "same" (goto BigNumberBase_callSame) else (goto BigNumberBase_callSimple)
:BigNumberBase_callSame
set BigNumberBase_dlllib=%2
call "%CLOUD_REBUILD%" BigNumberBase %BigNumberBase_bit% %BigNumberBase_dlllib% %BigNumberBase_debugRelease% %BigNumberBase_allSame%
goto BigNumberBase_end
:BigNumberBase_callSimple
call "%CLOUD_REBUILD%" BigNumberBase %BigNumberBase_bit% %BigNumberBase_dlllib% %BigNumberBase_debugRelease%
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