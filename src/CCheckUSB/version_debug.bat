set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::如果需要获取依赖dll-----------------------------------------------------------
call %xueyelingshuang%src\..relyproj..\version_debug.bat

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\
::如果需要获取依赖dll-----------------------------------------------------------

::编译工程
call "%bat%scripts\rebuild_CCheckUSB.py" debug

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::向公共部分提供文件，有的时候可能没有inl文件
xcopy /y /i /r /s "%bat%CCheckUSB\src\*.h"   "%xueyelingshuang%include\CCheckUSB\"
xcopy /y /i /r /s "%bat%CCheckUSB\src\*.inl" "%xueyelingshuang%include\CCheckUSB\"


