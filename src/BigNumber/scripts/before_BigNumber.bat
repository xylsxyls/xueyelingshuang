
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
set BigNumberBase_allSame=same
if "%4" == "same" (goto BigNumberBase_callSame) else (goto BigNumberBase_callSimple)
:BigNumberBase_callSame
set BigNumberBase_dlllib=lib
call "%CLOUD_REBUILD%" BigNumberBase %BigNumberBase_bit% %BigNumberBase_dlllib% %BigNumberBase_debugRelease% %BigNumberBase_allSame%
goto BigNumberBase_end
:BigNumberBase_callSimple
call "%CLOUD_REBUILD%" BigNumberBase %BigNumberBase_bit% %BigNumberBase_dlllib% %BigNumberBase_debugRelease%
goto BigNumberBase_end
:BigNumberBase_end
