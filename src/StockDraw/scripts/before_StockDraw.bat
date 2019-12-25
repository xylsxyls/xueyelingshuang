
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