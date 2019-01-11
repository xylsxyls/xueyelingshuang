
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
set ProcessWork_dlllib=lib
set ProcessWork_bit=%1
set ProcessWork_debugRelease=%3
set ProcessWork_allSame=%4
if "%4" == "same" (goto ProcessWork_callSame) else (goto ProcessWork_callSimple)
:ProcessWork_callSame
set ProcessWork_dlllib=%2
call "%CLOUD_REBUILD%" ProcessWork %ProcessWork_bit% %ProcessWork_dlllib% %ProcessWork_debugRelease% %ProcessWork_allSame%
goto ProcessWork_end
:ProcessWork_callSimple
call "%CLOUD_REBUILD%" ProcessWork %ProcessWork_bit% %ProcessWork_dlllib% %ProcessWork_debugRelease%
goto ProcessWork_end
:ProcessWork_end

::--------------------------------------------------------------------
set ProtoMessage_dlllib=lib
set ProtoMessage_bit=%1
set ProtoMessage_debugRelease=%3
set ProtoMessage_allSame=%4
if "%4" == "same" (goto ProtoMessage_callSame) else (goto ProtoMessage_callSimple)
:ProtoMessage_callSame
set ProtoMessage_dlllib=%2
call "%CLOUD_REBUILD%" ProtoMessage %ProtoMessage_bit% %ProtoMessage_dlllib% %ProtoMessage_debugRelease% %ProtoMessage_allSame%
goto ProtoMessage_end
:ProtoMessage_callSimple
call "%CLOUD_REBUILD%" ProtoMessage %ProtoMessage_bit% %ProtoMessage_dlllib% %ProtoMessage_debugRelease%
goto ProtoMessage_end
:ProtoMessage_end