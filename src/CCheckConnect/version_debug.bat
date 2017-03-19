set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::如果需要获取依赖dll-----------------------------------------------------------
call "%xueyelingshuang%src\CLoadDLL\version_debug.bat"

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\
::如果需要获取依赖dll-----------------------------------------------------------

::编译工程
call "%bat%scripts\rebuild_CCheckConnect.py" debug

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::向公共部分提供文件，有的时候可能没有inl文件
xcopy /y /i /r /s "%bat%CCheckConnect\src\*.h"   "%xueyelingshuang%include\CCheckConnect\"
xcopy /y /i /r /s "%bat%CCheckConnect\src\*.inl" "%xueyelingshuang%include\CCheckConnect\"

::删除临时文件
del "%bat%@AutomationLog.txt"
del "%bat%scripts\@AutomationLog.txt"
del "%bat%scripts\msbuild.log"
rmdir /q /s "%bat%CCheckConnect\vcxproj\Debug\"

::删除ilk，exp
del "%xueyelingshuang%lib\CCheckConnectd.ilk"
del "%xueyelingshuang%lib\CCheckConnectd.exp"