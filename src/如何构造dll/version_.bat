set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::如果需要获取依赖dll-----------------------------------------------------------
call "%xueyelingshuang%src\..relyproj..\version_debug.bat"

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\
::如果需要获取依赖dll-----------------------------------------------------------

::编译工程
call "%bat%scripts\rebuild_..proj...py" debug

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::向公共部分提供文件，有的时候可能没有inl文件
xcopy /y /i /r /s "%bat%..proj..\src\*.h"   "%xueyelingshuang%include\..proj..\"
xcopy /y /i /r /s "%bat%..proj..\src\*.inl" "%xueyelingshuang%include\..proj..\"

::删除临时文件
del "%bat%@AutomationLog.txt"
del "%bat%scripts\@AutomationLog.txt"
del "%bat%scripts\msbuild.log"
rmdir /q /s "%bat%..proj..\vcxproj\Debug\"

::删除ilk，exp
del "%xueyelingshuang%lib\..proj..d.ilk"
del "%xueyelingshuang%lib\..proj..d.exp"


::release-------------------------------------------------------------------------

set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::如果需要获取依赖dll-----------------------------------------------------------
call "%xueyelingshuang%src\..relyproj..\version_release.bat"

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\
::如果需要获取依赖dll-----------------------------------------------------------

::编译工程
call "%bat%scripts\rebuild_..proj...py"

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::向公共部分提供文件，有的时候可能没有inl文件
xcopy /y /i /r /s "%bat%..proj..\src\*.h"   "%xueyelingshuang%include\..proj..\"
xcopy /y /i /r /s "%bat%..proj..\src\*.inl" "%xueyelingshuang%include\..proj..\"

::删除临时文件
del "%bat%@AutomationLog.txt"
del "%bat%scripts\@AutomationLog.txt"
del "%bat%scripts\msbuild.log"
rmdir /q /s "%bat%..proj..\vcxproj\Release\"

::删除ilk，exp
del "%xueyelingshuang%lib\..proj...ilk"
del "%xueyelingshuang%lib\..proj...exp"