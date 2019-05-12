
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
set StockFund_dlllib=lib
set StockFund_bit=%1
set StockFund_debugRelease=%3
set StockFund_allSame=%4
if "%4" == "same" (goto StockFund_callSame) else (goto StockFund_callSimple)
:StockFund_callSame
set StockFund_dlllib=%2
call "%CLOUD_REBUILD%" StockFund %StockFund_bit% %StockFund_dlllib% %StockFund_debugRelease% %StockFund_allSame%
goto StockFund_end
:StockFund_callSimple
call "%CLOUD_REBUILD%" StockFund %StockFund_bit% %StockFund_dlllib% %StockFund_debugRelease%
goto StockFund_end
:StockFund_end

::--------------------------------------------------------------------
set StockIndex_dlllib=lib
set StockIndex_bit=%1
set StockIndex_debugRelease=%3
set StockIndex_allSame=%4
if "%4" == "same" (goto StockIndex_callSame) else (goto StockIndex_callSimple)
:StockIndex_callSame
set StockIndex_dlllib=%2
call "%CLOUD_REBUILD%" StockIndex %StockIndex_bit% %StockIndex_dlllib% %StockIndex_debugRelease% %StockIndex_allSame%
goto StockIndex_end
:StockIndex_callSimple
call "%CLOUD_REBUILD%" StockIndex %StockIndex_bit% %StockIndex_dlllib% %StockIndex_debugRelease%
goto StockIndex_end
:StockIndex_end

::--------------------------------------------------------------------
set CStopWatch_dlllib=lib
set CStopWatch_bit=%1
set CStopWatch_debugRelease=%3
set CStopWatch_allSame=%4
if "%4" == "same" (goto CStopWatch_callSame) else (goto CStopWatch_callSimple)
:CStopWatch_callSame
set CStopWatch_dlllib=%2
call "%CLOUD_REBUILD%" CStopWatch %CStopWatch_bit% %CStopWatch_dlllib% %CStopWatch_debugRelease% %CStopWatch_allSame%
goto CStopWatch_end
:CStopWatch_callSimple
call "%CLOUD_REBUILD%" CStopWatch %CStopWatch_bit% %CStopWatch_dlllib% %CStopWatch_debugRelease%
goto CStopWatch_end
:CStopWatch_end