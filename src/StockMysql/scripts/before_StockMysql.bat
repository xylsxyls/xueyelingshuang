
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