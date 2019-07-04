
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
set QtControls_dlllib=lib
set QtControls_bit=%1
set QtControls_debugRelease=%3
set QtControls_allSame=%4
if "%4" == "same" (goto QtControls_callSame) else (goto QtControls_callSimple)
:QtControls_callSame
set QtControls_dlllib=%2
call "%CLOUD_REBUILD%" QtControls %QtControls_bit% %QtControls_dlllib% %QtControls_debugRelease% %QtControls_allSame%
goto QtControls_end
:QtControls_callSimple
call "%CLOUD_REBUILD%" QtControls %QtControls_bit% %QtControls_dlllib% %QtControls_debugRelease%
goto QtControls_end
:QtControls_end

::--------------------------------------------------------------------
set ManagerBase_dlllib=lib
set ManagerBase_bit=%1
set ManagerBase_debugRelease=%3
set ManagerBase_allSame=%4
if "%4" == "same" (goto ManagerBase_callSame) else (goto ManagerBase_callSimple)
:ManagerBase_callSame
set ManagerBase_dlllib=%2
call "%CLOUD_REBUILD%" ManagerBase %ManagerBase_bit% %ManagerBase_dlllib% %ManagerBase_debugRelease% %ManagerBase_allSame%
goto ManagerBase_end
:ManagerBase_callSimple
call "%CLOUD_REBUILD%" ManagerBase %ManagerBase_bit% %ManagerBase_dlllib% %ManagerBase_debugRelease%
goto ManagerBase_end
:ManagerBase_end