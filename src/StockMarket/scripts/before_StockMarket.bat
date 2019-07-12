
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
set StockMysql_dlllib=lib
set StockMysql_bit=%1
set StockMysql_debugRelease=%3
set StockMysql_allSame=%4
if "%4" == "same" (goto StockMysql_callSame) else (goto StockMysql_callSimple)
:StockMysql_callSame
set StockMysql_dlllib=%2
call "%CLOUD_REBUILD%" StockMysql %StockMysql_bit% %StockMysql_dlllib% %StockMysql_debugRelease% %StockMysql_allSame%
goto StockMysql_end
:StockMysql_callSimple
call "%CLOUD_REBUILD%" StockMysql %StockMysql_bit% %StockMysql_dlllib% %StockMysql_debugRelease%
goto StockMysql_end
:StockMysql_end

::--------------------------------------------------------------------
set Ctxt_dlllib=lib
set Ctxt_bit=%1
set Ctxt_debugRelease=%3
set Ctxt_allSame=%4
if "%4" == "same" (goto Ctxt_callSame) else (goto Ctxt_callSimple)
:Ctxt_callSame
set Ctxt_dlllib=%2
call "%CLOUD_REBUILD%" Ctxt %Ctxt_bit% %Ctxt_dlllib% %Ctxt_debugRelease% %Ctxt_allSame%
goto Ctxt_end
:Ctxt_callSimple
call "%CLOUD_REBUILD%" Ctxt %Ctxt_bit% %Ctxt_dlllib% %Ctxt_debugRelease%
goto Ctxt_end
:Ctxt_end

::--------------------------------------------------------------------
set CGetPath_dlllib=lib
set CGetPath_bit=%1
set CGetPath_debugRelease=%3
set CGetPath_allSame=%4
if "%4" == "same" (goto CGetPath_callSame) else (goto CGetPath_callSimple)
:CGetPath_callSame
set CGetPath_dlllib=%2
call "%CLOUD_REBUILD%" CGetPath %CGetPath_bit% %CGetPath_dlllib% %CGetPath_debugRelease% %CGetPath_allSame%
goto CGetPath_end
:CGetPath_callSimple
call "%CLOUD_REBUILD%" CGetPath %CGetPath_bit% %CGetPath_dlllib% %CGetPath_debugRelease%
goto CGetPath_end
:CGetPath_end