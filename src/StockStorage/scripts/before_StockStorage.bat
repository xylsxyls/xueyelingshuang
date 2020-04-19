
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
set StockSolution_dlllib=lib
set StockSolution_bit=%1
set StockSolution_debugRelease=%3
set StockSolution_allSame=%4
if "%4" == "same" (goto StockSolution_callSame) else (goto StockSolution_callSimple)
:StockSolution_callSame
set StockSolution_dlllib=%2
call "%CLOUD_REBUILD%" StockSolution %StockSolution_bit% %StockSolution_dlllib% %StockSolution_debugRelease% %StockSolution_allSame%
goto StockSolution_end
:StockSolution_callSimple
call "%CLOUD_REBUILD%" StockSolution %StockSolution_bit% %StockSolution_dlllib% %StockSolution_debugRelease%
goto StockSolution_end
:StockSolution_end