
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
set SqlString_dlllib=lib
set SqlString_bit=%1
set SqlString_debugRelease=%3
set SqlString_allSame=%4
if "%4" == "same" (goto SqlString_callSame) else (goto SqlString_callSimple)
:SqlString_callSame
set SqlString_dlllib=%2
call "%CLOUD_REBUILD%" SqlString %SqlString_bit% %SqlString_dlllib% %SqlString_debugRelease% %SqlString_allSame%
goto SqlString_end
:SqlString_callSimple
call "%CLOUD_REBUILD%" SqlString %SqlString_bit% %SqlString_dlllib% %SqlString_debugRelease%
goto SqlString_end
:SqlString_end

::--------------------------------------------------------------------
set CCharset_dlllib=lib
set CCharset_bit=%1
set CCharset_debugRelease=%3
set CCharset_allSame=%4
if "%4" == "same" (goto CCharset_callSame) else (goto CCharset_callSimple)
:CCharset_callSame
set CCharset_dlllib=%2
call "%CLOUD_REBUILD%" CCharset %CCharset_bit% %CCharset_dlllib% %CCharset_debugRelease% %CCharset_allSame%
goto CCharset_end
:CCharset_callSimple
call "%CLOUD_REBUILD%" CCharset %CCharset_bit% %CCharset_dlllib% %CCharset_debugRelease%
goto CCharset_end
:CCharset_end