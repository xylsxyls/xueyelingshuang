
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
set StockMarket_dlllib=lib
set StockMarket_bit=%1
set StockMarket_debugRelease=%3
set StockMarket_allSame=%4
if "%4" == "same" (goto StockMarket_callSame) else (goto StockMarket_callSimple)
:StockMarket_callSame
set StockMarket_dlllib=%2
call "%CLOUD_REBUILD%" StockMarket %StockMarket_bit% %StockMarket_dlllib% %StockMarket_debugRelease% %StockMarket_allSame%
goto StockMarket_end
:StockMarket_callSimple
call "%CLOUD_REBUILD%" StockMarket %StockMarket_bit% %StockMarket_dlllib% %StockMarket_debugRelease%
goto StockMarket_end
:StockMarket_end

::--------------------------------------------------------------------
set StockCharge_dlllib=lib
set StockCharge_bit=%1
set StockCharge_debugRelease=%3
set StockCharge_allSame=%4
if "%4" == "same" (goto StockCharge_callSame) else (goto StockCharge_callSimple)
:StockCharge_callSame
set StockCharge_dlllib=%2
call "%CLOUD_REBUILD%" StockCharge %StockCharge_bit% %StockCharge_dlllib% %StockCharge_debugRelease% %StockCharge_allSame%
goto StockCharge_end
:StockCharge_callSimple
call "%CLOUD_REBUILD%" StockCharge %StockCharge_bit% %StockCharge_dlllib% %StockCharge_debugRelease%
goto StockCharge_end
:StockCharge_end