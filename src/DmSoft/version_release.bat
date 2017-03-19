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
call "%bat%scripts\rebuild_DmSoft.py"

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::向公共部分提供文件，有的时候可能没有inl文件
xcopy /y /i /r /s "%bat%DmSoft\src\*.h"   "%xueyelingshuang%include\DmSoft\"
xcopy /y /i /r /s "%bat%DmSoft\src\*.inl" "%xueyelingshuang%include\DmSoft\"
del "%xueyelingshuang%include\DmSoft\Cdmsoft.h"
xcopy /y /i /r /s "%xueyelingshuang%tools\大漠插件\dm.dll" "%xueyelingshuang%lib\"

::删除临时文件
del "%bat%@AutomationLog.txt"
del "%bat%scripts\@AutomationLog.txt"
del "%bat%scripts\msbuild.log"
rmdir /q /s "%bat%DmSoft\vcxproj\Release\"

::删除ilk，exp
del "%xueyelingshuang%lib\DmSoft.ilk"
del "%xueyelingshuang%lib\DmSoft.exp"