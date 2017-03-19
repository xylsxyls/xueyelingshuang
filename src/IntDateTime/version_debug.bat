set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::如果需要获取依赖dll-----------------------------------------------------------
call "%xueyelingshuang%src\CStringManager\version_debug.bat"

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\
::如果需要获取依赖dll-----------------------------------------------------------

::编译工程
call "%bat%scripts\rebuild_IntDateTime.py" debug

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::向公共部分提供文件，有的时候可能没有inl文件
xcopy /y /i /r /s "%bat%IntDateTime\src\*.h"   "%xueyelingshuang%include\IntDateTime\"
xcopy /y /i /r /s "%bat%IntDateTime\src\*.inl" "%xueyelingshuang%include\IntDateTime\"

::删除临时文件
del "%bat%@AutomationLog.txt"
del "%bat%scripts\@AutomationLog.txt"
del "%bat%scripts\msbuild.log"
rmdir /q /s "%bat%IntDateTime\vcxproj\Debug\"

::删除ilk，exp
del "%xueyelingshuang%lib\IntDateTimed.ilk"
del "%xueyelingshuang%lib\IntDateTimed.exp"