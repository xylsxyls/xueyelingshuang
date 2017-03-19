set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::获取第三方库
call "%bat%public.bat"

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::编译工程
call "%bat%scripts\rebuild_CMysqlAPI.py" debug

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::向公共部分提供文件
xcopy /y /i /r /s "%bat%CMysqlAPI\src\*.h" "%xueyelingshuang%include\CMysqlAPI\"
xcopy /y /i /r /s "%bat%CMysqlAPI\src\*.inl" "%xueyelingshuang%include\CMysqlAPI\"

xcopy /y /i /r /s "%bat%CMysqlAPI\props\*.props" "%xueyelingshuang%props\"

::删除临时文件
del "%bat%@AutomationLog.txt"
del "%bat%scripts\@AutomationLog.txt"
del "%bat%scripts\msbuild.log"
rmdir /q /s "%bat%CMysqlAPI\vcxproj\Debug\"

::删除ilk，exp
del "%xueyelingshuang%lib\CMysqlAPId.ilk"
del "%xueyelingshuang%lib\CMysqlAPId.exp"