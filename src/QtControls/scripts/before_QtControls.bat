
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
set QtCore_dlllib=lib
set QtCore_bit=%1
set QtCore_debugRelease=%3
set QtCore_allSame=%4
if "%4" == "same" (goto QtCore_callSame) else (goto QtCore_callSimple)
:QtCore_callSame
set QtCore_dlllib=%2
call "%CLOUD_REBUILD%" QtCore %QtCore_bit% %QtCore_dlllib% %QtCore_debugRelease% %QtCore_allSame%
goto QtCore_end
:QtCore_callSimple
call "%CLOUD_REBUILD%" QtCore %QtCore_bit% %QtCore_dlllib% %QtCore_debugRelease%
goto QtCore_end
:QtCore_end