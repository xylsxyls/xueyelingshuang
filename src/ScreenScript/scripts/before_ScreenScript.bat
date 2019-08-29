
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
set CMouse_dlllib=lib
set CMouse_bit=%1
set CMouse_debugRelease=%3
set CMouse_allSame=%4
if "%4" == "same" (goto CMouse_callSame) else (goto CMouse_callSimple)
:CMouse_callSame
set CMouse_dlllib=%2
call "%CLOUD_REBUILD%" CMouse %CMouse_bit% %CMouse_dlllib% %CMouse_debugRelease% %CMouse_allSame%
goto CMouse_end
:CMouse_callSimple
call "%CLOUD_REBUILD%" CMouse %CMouse_bit% %CMouse_dlllib% %CMouse_debugRelease%
goto CMouse_end
:CMouse_end

::--------------------------------------------------------------------
set CScreen_dlllib=lib
set CScreen_bit=%1
set CScreen_debugRelease=%3
set CScreen_allSame=%4
if "%4" == "same" (goto CScreen_callSame) else (goto CScreen_callSimple)
:CScreen_callSame
set CScreen_dlllib=%2
call "%CLOUD_REBUILD%" CScreen %CScreen_bit% %CScreen_dlllib% %CScreen_debugRelease% %CScreen_allSame%
goto CScreen_end
:CScreen_callSimple
call "%CLOUD_REBUILD%" CScreen %CScreen_bit% %CScreen_dlllib% %CScreen_debugRelease%
goto CScreen_end
:CScreen_end

::--------------------------------------------------------------------
set CSystem_dlllib=lib
set CSystem_bit=%1
set CSystem_debugRelease=%3
set CSystem_allSame=%4
if "%4" == "same" (goto CSystem_callSame) else (goto CSystem_callSimple)
:CSystem_callSame
set CSystem_dlllib=%2
call "%CLOUD_REBUILD%" CSystem %CSystem_bit% %CSystem_dlllib% %CSystem_debugRelease% %CSystem_allSame%
goto CSystem_end
:CSystem_callSimple
call "%CLOUD_REBUILD%" CSystem %CSystem_bit% %CSystem_dlllib% %CSystem_debugRelease%
goto CSystem_end
:CSystem_end