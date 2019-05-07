
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
set MysqlCpp_dlllib=lib
set MysqlCpp_bit=%1
set MysqlCpp_debugRelease=%3
set MysqlCpp_allSame=%4
if "%4" == "same" (goto MysqlCpp_callSame) else (goto MysqlCpp_callSimple)
:MysqlCpp_callSame
set MysqlCpp_dlllib=%2
call "%CLOUD_REBUILD%" MysqlCpp %MysqlCpp_bit% %MysqlCpp_dlllib% %MysqlCpp_debugRelease% %MysqlCpp_allSame%
goto MysqlCpp_end
:MysqlCpp_callSimple
call "%CLOUD_REBUILD%" MysqlCpp %MysqlCpp_bit% %MysqlCpp_dlllib% %MysqlCpp_debugRelease%
goto MysqlCpp_end
:MysqlCpp_end

::--------------------------------------------------------------------
set IntDateTime_dlllib=lib
set IntDateTime_bit=%1
set IntDateTime_debugRelease=%3
set IntDateTime_allSame=%4
if "%4" == "same" (goto IntDateTime_callSame) else (goto IntDateTime_callSimple)
:IntDateTime_callSame
set IntDateTime_dlllib=%2
call "%CLOUD_REBUILD%" IntDateTime %IntDateTime_bit% %IntDateTime_dlllib% %IntDateTime_debugRelease% %IntDateTime_allSame%
goto IntDateTime_end
:IntDateTime_callSimple
call "%CLOUD_REBUILD%" IntDateTime %IntDateTime_bit% %IntDateTime_dlllib% %IntDateTime_debugRelease%
goto IntDateTime_end
:IntDateTime_end

::--------------------------------------------------------------------
set BigNumber_dlllib=lib
set BigNumber_bit=%1
set BigNumber_debugRelease=%3
set BigNumber_allSame=%4
if "%4" == "same" (goto BigNumber_callSame) else (goto BigNumber_callSimple)
:BigNumber_callSame
set BigNumber_dlllib=%2
call "%CLOUD_REBUILD%" BigNumber %BigNumber_bit% %BigNumber_dlllib% %BigNumber_debugRelease% %BigNumber_allSame%
goto BigNumber_end
:BigNumber_callSimple
call "%CLOUD_REBUILD%" BigNumber %BigNumber_bit% %BigNumber_dlllib% %BigNumber_debugRelease%
goto BigNumber_end
:BigNumber_end

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