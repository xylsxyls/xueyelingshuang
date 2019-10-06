
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
set StockTrade_dlllib=lib
set StockTrade_bit=%1
set StockTrade_debugRelease=%3
set StockTrade_allSame=%4
if "%4" == "same" (goto StockTrade_callSame) else (goto StockTrade_callSimple)
:StockTrade_callSame
set StockTrade_dlllib=%2
call "%CLOUD_REBUILD%" StockTrade %StockTrade_bit% %StockTrade_dlllib% %StockTrade_debugRelease% %StockTrade_allSame%
goto StockTrade_end
:StockTrade_callSimple
call "%CLOUD_REBUILD%" StockTrade %StockTrade_bit% %StockTrade_dlllib% %StockTrade_debugRelease%
goto StockTrade_end
:StockTrade_end