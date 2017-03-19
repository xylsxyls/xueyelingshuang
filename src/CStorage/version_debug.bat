set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::拷贝boost动态库
xcopy /y /i /r /s "%BOOST_ROOT%\lib32-msvc-11.0\*.dll" "%xueyelingshuang%lib\"

::如果需要获取依赖dll-----------------------------------------------------------
call "%xueyelingshuang%src\..relyproj..\version_debug.bat"

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\
::如果需要获取依赖dll-----------------------------------------------------------

::编译工程
call "%bat%scripts\rebuild_CStorage.py" debug

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::向公共部分提供文件，有的时候可能没有inl文件
xcopy /y /i /r /s "%bat%CStorage\src\*.h"   "%xueyelingshuang%include\CStorage\"
xcopy /y /i /r /s "%bat%CStorage\src\*.inl" "%xueyelingshuang%include\CStorage\"
xcopy /y /i /r /s "%bat%CStorage\props\*.props" "%xueyelingshuang%props\"

::删除临时文件
del "%bat%@AutomationLog.txt"
del "%bat%scripts\@AutomationLog.txt"
del "%bat%scripts\msbuild.log"
rmdir /q /s "%bat%CStorage\vcxproj\Debug\"

::删除ilk，exp
del "%xueyelingshuang%lib\CStoraged.ilk"
del "%xueyelingshuang%lib\CStoraged.exp"
