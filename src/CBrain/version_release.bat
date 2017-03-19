::release-------------------------------------------------------------------------

set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::如果需要获取依赖dll-----------------------------------------------------------
call "%xueyelingshuang%src\CTask\version_release.bat"

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\
::如果需要获取依赖dll-----------------------------------------------------------

::编译工程
call "%bat%scripts\rebuild_CBrain.py"

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::向公共部分提供文件，有的时候可能没有inl文件
xcopy /y /i /r /s "%bat%CBrain\src\*.h"   "%xueyelingshuang%include\CBrain\"
xcopy /y /i /r /s "%bat%CBrain\src\*.inl" "%xueyelingshuang%include\CBrain\"

::删除临时文件
del "%bat%@AutomationLog.txt"
del "%bat%scripts\@AutomationLog.txt"
del "%bat%scripts\msbuild.log"
rmdir /q /s "%bat%CBrain\vcxproj\Release\"

::删除ilk，exp
del "%xueyelingshuang%lib\CBrain.ilk"
del "%xueyelingshuang%lib\CBrain.exp"