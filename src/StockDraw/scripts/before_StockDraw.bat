
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
set StockIndicator_dlllib=lib
set StockIndicator_bit=%1
set StockIndicator_debugRelease=%3
set StockIndicator_allSame=%4
if "%4" == "same" (goto StockIndicator_callSame) else (goto StockIndicator_callSimple)
:StockIndicator_callSame
set StockIndicator_dlllib=%2
call "%CLOUD_REBUILD%" StockIndicator %StockIndicator_bit% %StockIndicator_dlllib% %StockIndicator_debugRelease% %StockIndicator_allSame%
goto StockIndicator_end
:StockIndicator_callSimple
call "%CLOUD_REBUILD%" StockIndicator %StockIndicator_bit% %StockIndicator_dlllib% %StockIndicator_debugRelease%
goto StockIndicator_end
:StockIndicator_end

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
set ConfigManager_dlllib=lib
set ConfigManager_bit=%1
set ConfigManager_debugRelease=%3
set ConfigManager_allSame=%4
if "%4" == "same" (goto ConfigManager_callSame) else (goto ConfigManager_callSimple)
:ConfigManager_callSame
set ConfigManager_dlllib=%2
call "%CLOUD_REBUILD%" ConfigManager %ConfigManager_bit% %ConfigManager_dlllib% %ConfigManager_debugRelease% %ConfigManager_allSame%
goto ConfigManager_end
:ConfigManager_callSimple
call "%CLOUD_REBUILD%" ConfigManager %ConfigManager_bit% %ConfigManager_dlllib% %ConfigManager_debugRelease%
goto ConfigManager_end
:ConfigManager_end

::--------------------------------------------------------------------
set DialogManager_dlllib=lib
set DialogManager_bit=%1
set DialogManager_debugRelease=%3
set DialogManager_allSame=%4
if "%4" == "same" (goto DialogManager_callSame) else (goto DialogManager_callSimple)
:DialogManager_callSame
set DialogManager_dlllib=%2
call "%CLOUD_REBUILD%" DialogManager %DialogManager_bit% %DialogManager_dlllib% %DialogManager_debugRelease% %DialogManager_allSame%
goto DialogManager_end
:DialogManager_callSimple
call "%CLOUD_REBUILD%" DialogManager %DialogManager_bit% %DialogManager_dlllib% %DialogManager_debugRelease%
goto DialogManager_end
:DialogManager_end