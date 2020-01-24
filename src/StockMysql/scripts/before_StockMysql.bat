
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
set HiRedis_dlllib=lib
set HiRedis_bit=%1
set HiRedis_debugRelease=%3
set HiRedis_allSame=%4
if "%4" == "same" (goto HiRedis_callSame) else (goto HiRedis_callSimple)
:HiRedis_callSame
set HiRedis_dlllib=%2
call "%CLOUD_REBUILD%" HiRedis %HiRedis_bit% %HiRedis_dlllib% %HiRedis_debugRelease% %HiRedis_allSame%
goto HiRedis_end
:HiRedis_callSimple
call "%CLOUD_REBUILD%" HiRedis %HiRedis_bit% %HiRedis_dlllib% %HiRedis_debugRelease%
goto HiRedis_end
:HiRedis_end

::--------------------------------------------------------------------
set CBase64_dlllib=lib
set CBase64_bit=%1
set CBase64_debugRelease=%3
set CBase64_allSame=%4
if "%4" == "same" (goto CBase64_callSame) else (goto CBase64_callSimple)
:CBase64_callSame
set CBase64_dlllib=%2
call "%CLOUD_REBUILD%" CBase64 %CBase64_bit% %CBase64_dlllib% %CBase64_debugRelease% %CBase64_allSame%
goto CBase64_end
:CBase64_callSimple
call "%CLOUD_REBUILD%" CBase64 %CBase64_bit% %CBase64_dlllib% %CBase64_debugRelease%
goto CBase64_end
:CBase64_end