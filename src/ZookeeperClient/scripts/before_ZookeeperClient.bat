
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

xcopy /y /i /r /s "%xueyelingshuang%tools\zookeeper\include\*" "%xueyelingshuang%include\zookeeper\"

if "%1" == "32" (goto ZookeeperClient_32) else (goto ZookeeperClient_64)
:ZookeeperClient_32
if "%2" == "release" (goto ZookeeperClient_release) else (goto ZookeeperClient_debug)
:ZookeeperClient_release
xcopy /y /i /r /s "%xueyelingshuang%tools\zookeeper\win32\release\*.lib" "%xueyelingshuang%lib\"
goto ZookeeperClient_end
:ZookeeperClient_64
if "%2" == "release" (goto ZookeeperClient_64_release) else (goto ZookeeperClient_64_debug)
:ZookeeperClient_64_debug
xcopy /y /i /r /s "%xueyelingshuang%tools\zookeeper\win64\debug\*.lib" "%xueyelingshuang%lib\"
goto ZookeeperClient_end
:ZookeeperClient_64_release
xcopy /y /i /r /s "%xueyelingshuang%tools\zookeeper\win64\release\*.lib" "%xueyelingshuang%lib\"
goto ZookeeperClient_end
:ZookeeperClient_end
goto ZookeeperClient_debug_release_end
:ZookeeperClient_debug
xcopy /y /i /r /s "%xueyelingshuang%tools\zookeeper\win32\debug\*.lib" "%xueyelingshuang%lib\"
goto ZookeeperClient_debug_release_end
:ZookeeperClient_debug_release_end

::--------------------------------------------------------------------
set Semaphore_dlllib=lib
set Semaphore_bit=%1
set Semaphore_debugRelease=%3
set Semaphore_allSame=%4
if "%4" == "same" (goto Semaphore_callSame) else (goto Semaphore_callSimple)
:Semaphore_callSame
set Semaphore_dlllib=%2
call "%CLOUD_REBUILD%" Semaphore %Semaphore_bit% %Semaphore_dlllib% %Semaphore_debugRelease% %Semaphore_allSame%
goto Semaphore_end
:Semaphore_callSimple
call "%CLOUD_REBUILD%" Semaphore %Semaphore_bit% %Semaphore_dlllib% %Semaphore_debugRelease%
goto Semaphore_end
:Semaphore_end