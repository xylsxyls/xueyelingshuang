
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
set Variant_dlllib=lib
set Variant_bit=%1
set Variant_debugRelease=%3
set Variant_allSame=%4
if "%4" == "same" (goto Variant_callSame) else (goto Variant_callSimple)
:Variant_callSame
set Variant_dlllib=%2
call "%CLOUD_REBUILD%" Variant %Variant_bit% %Variant_dlllib% %Variant_debugRelease% %Variant_allSame%
goto Variant_end
:Variant_callSimple
call "%CLOUD_REBUILD%" Variant %Variant_bit% %Variant_dlllib% %Variant_debugRelease%
goto Variant_end
:Variant_end

::--------------------------------------------------------------------
set bat=%~dp0
set xueyelingshuang=%bat%..\..\..\

xcopy /y /i /r /s "%xueyelingshuang%tools\protobuf\google\*" "%xueyelingshuang%include\google\"
xcopy /y /i /r /s "%xueyelingshuang%tools\protobuf\*.lib" "%xueyelingshuang%lib\"

::--------------------------------------------------------------------
set bat=%~dp0
set xueyelingshuang=%bat%..\..\..\

cd %xueyelingshuang%tools\protobuf\
protoc -I="%xueyelingshuang%src\ProtoMessage\ProtoMessage\src" --cpp_out="%xueyelingshuang%src\ProtoMessage\ProtoMessage\src" "%xueyelingshuang%src\ProtoMessage\ProtoMessage\src\ProtoMessage.proto"