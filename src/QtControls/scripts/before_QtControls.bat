
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
set QssString_dlllib=lib
set QssString_bit=%1
set QssString_debugRelease=%3
set QssString_allSame=%4
if "%4" == "same" (goto QssString_callSame) else (goto QssString_callSimple)
:QssString_callSame
set QssString_dlllib=%2
call "%CLOUD_REBUILD%" QssString %QssString_bit% %QssString_dlllib% %QssString_debugRelease% %QssString_allSame%
goto QssString_end
:QssString_callSimple
call "%CLOUD_REBUILD%" QssString %QssString_bit% %QssString_dlllib% %QssString_debugRelease%
goto QssString_end
:QssString_end

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

::--------------------------------------------------------------------
set EO_XmlSax2Parser_dlllib=lib
set EO_XmlSax2Parser_bit=%1
set EO_XmlSax2Parser_debugRelease=%3
set EO_XmlSax2Parser_allSame=%4
if "%4" == "same" (goto EO_XmlSax2Parser_callSame) else (goto EO_XmlSax2Parser_callSimple)
:EO_XmlSax2Parser_callSame
set EO_XmlSax2Parser_dlllib=%2
call "%CLOUD_REBUILD%" EO_XmlSax2Parser %EO_XmlSax2Parser_bit% %EO_XmlSax2Parser_dlllib% %EO_XmlSax2Parser_debugRelease% %EO_XmlSax2Parser_allSame%
goto EO_XmlSax2Parser_end
:EO_XmlSax2Parser_callSimple
call "%CLOUD_REBUILD%" EO_XmlSax2Parser %EO_XmlSax2Parser_bit% %EO_XmlSax2Parser_dlllib% %EO_XmlSax2Parser_debugRelease%
goto EO_XmlSax2Parser_end
:EO_XmlSax2Parser_end