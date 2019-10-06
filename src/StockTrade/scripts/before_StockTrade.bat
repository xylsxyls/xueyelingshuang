
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
set StockStrategy_dlllib=lib
set StockStrategy_bit=%1
set StockStrategy_debugRelease=%3
set StockStrategy_allSame=%4
if "%4" == "same" (goto StockStrategy_callSame) else (goto StockStrategy_callSimple)
:StockStrategy_callSame
set StockStrategy_dlllib=%2
call "%CLOUD_REBUILD%" StockStrategy %StockStrategy_bit% %StockStrategy_dlllib% %StockStrategy_debugRelease% %StockStrategy_allSame%
goto StockStrategy_end
:StockStrategy_callSimple
call "%CLOUD_REBUILD%" StockStrategy %StockStrategy_bit% %StockStrategy_dlllib% %StockStrategy_debugRelease%
goto StockStrategy_end
:StockStrategy_end