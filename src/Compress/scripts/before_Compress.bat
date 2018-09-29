
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
set bat=%~dp0
set xueyelingshuang=%bat%..\..\..\

xcopy /y /i /r /s "%xueyelingshuang%tools\zlibrelease\zconf.h" "%xueyelingshuang%include\"
xcopy /y /i /r /s "%xueyelingshuang%tools\zlibrelease\zlib.h" "%xueyelingshuang%include\"

xcopy /y /i /r /s "%xueyelingshuang%tools\zlibrelease\zlibstat_32.lib" "%xueyelingshuang%lib\"
xcopy /y /i /r /s "%xueyelingshuang%tools\zlibrelease\zlibstat_64.lib" "%xueyelingshuang%lib\"