set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::编译工程
call "%bat%scripts\rebuild_CStopWatch.py" debug

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::向公共部分提供文件
xcopy /y /i /r /s "%bat%CStopWatch\src\*.h" "%xueyelingshuang%include\CStopWatch\"

::删除临时文件
del "%bat%@AutomationLog.txt"
del "%bat%scripts\@AutomationLog.txt"
del "%bat%scripts\msbuild.log"
rmdir /q /s "%bat%CStopWatch\vcxproj\Debug\"

::删除ilk，exp
del "%xueyelingshuang%lib\CStopWatchd.ilk"
del "%xueyelingshuang%lib\CStopWatchd.exp"