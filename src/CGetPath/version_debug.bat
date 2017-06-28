set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::如果需要获取依赖dll-----------------------------------------------------------
call "%xueyelingshuang%src\CStringManager\version_debug.bat"

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\
::如果需要获取依赖dll-----------------------------------------------------------
call "%xueyelingshuang%src\CSystem\version_debug.bat"

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\
::如果需要获取依赖dll-----------------------------------------------------------

::编译工程
call "%bat%scripts\rebuild_CGetPath.py" debug

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::向公共部分提供文件，有的时候可能没有inl文件
xcopy /y /i /r /s "%bat%CGetPath\src\*.h"   "%xueyelingshuang%include\CGetPath\"
xcopy /y /i /r /s "%bat%CGetPath\src\*.inl" "%xueyelingshuang%include\CGetPath\"

::删除临时文件
del "%bat%@AutomationLog.txt"
del "%bat%scripts\@AutomationLog.txt"
del "%bat%scripts\msbuild.log"
rmdir /q /s "%bat%CGetPath\vcxproj\Debug\"

::删除ilk，exp
del "%xueyelingshuang%lib\CGetPathd.ilk"
del "%xueyelingshuang%lib\CGetPathd.exp"
