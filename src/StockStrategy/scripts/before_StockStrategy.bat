
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
set StockForm_dlllib=lib
set StockForm_bit=%1
set StockForm_debugRelease=%3
set StockForm_allSame=%4
if "%4" == "same" (goto StockForm_callSame) else (goto StockForm_callSimple)
:StockForm_callSame
set StockForm_dlllib=%2
call "%CLOUD_REBUILD%" StockForm %StockForm_bit% %StockForm_dlllib% %StockForm_debugRelease% %StockForm_allSame%
goto StockForm_end
:StockForm_callSimple
call "%CLOUD_REBUILD%" StockForm %StockForm_bit% %StockForm_dlllib% %StockForm_debugRelease%
goto StockForm_end
:StockForm_end