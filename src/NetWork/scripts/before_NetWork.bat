
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
set CorrespondParam_dlllib=lib
set CorrespondParam_bit=%1
set CorrespondParam_debugRelease=%3
set CorrespondParam_allSame=%4
if "%4" == "same" (goto CorrespondParam_callSame) else (goto CorrespondParam_callSimple)
:CorrespondParam_callSame
set CorrespondParam_dlllib=%2
call "%CLOUD_REBUILD%" CorrespondParam %CorrespondParam_bit% %CorrespondParam_dlllib% %CorrespondParam_debugRelease% %CorrespondParam_allSame%
goto CorrespondParam_end
:CorrespondParam_callSimple
call "%CLOUD_REBUILD%" CorrespondParam %CorrespondParam_bit% %CorrespondParam_dlllib% %CorrespondParam_debugRelease%
goto CorrespondParam_end
:CorrespondParam_end

::--------------------------------------------------------------------
set LibuvTcp_dlllib=lib
set LibuvTcp_bit=%1
set LibuvTcp_debugRelease=%3
set LibuvTcp_allSame=%4
if "%4" == "same" (goto LibuvTcp_callSame) else (goto LibuvTcp_callSimple)
:LibuvTcp_callSame
set LibuvTcp_dlllib=%2
call "%CLOUD_REBUILD%" LibuvTcp %LibuvTcp_bit% %LibuvTcp_dlllib% %LibuvTcp_debugRelease% %LibuvTcp_allSame%
goto LibuvTcp_end
:LibuvTcp_callSimple
call "%CLOUD_REBUILD%" LibuvTcp %LibuvTcp_bit% %LibuvTcp_dlllib% %LibuvTcp_debugRelease%
goto LibuvTcp_end
:LibuvTcp_end