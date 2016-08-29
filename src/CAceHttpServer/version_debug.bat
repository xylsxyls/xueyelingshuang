set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::依赖tools
xcopy /y /i /r /s "%xueyelingshuang%tools\ACE_wrappers\ace\*"      "%xueyelingshuang%include\ace\"
xcopy /y /i /r /s "%xueyelingshuang%tools\ACE_wrappers\lib\vc11\*" "%xueyelingshuang%lib\"

::如果需要获取依赖dll-----------------------------------------------------------
call %xueyelingshuang%src\Cjson\version_debug.bat

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\
::如果需要获取依赖dll-----------------------------------------------------------
call %xueyelingshuang%src\CCharset\version_debug.bat

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\
::如果需要获取依赖dll-----------------------------------------------------------

::编译工程
call "%bat%scripts\rebuild_CAceHttpServer.py" debug

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::向公共部分提供文件，有的时候可能没有inl文件
xcopy /y /i /r /s "%bat%CAceHttpServer\src\*.h"   "%xueyelingshuang%include\CAceHttpServer\"
xcopy /y /i /r /s "%bat%CAceHttpServer\src\*.inl" "%xueyelingshuang%include\CAceHttpServer\"


