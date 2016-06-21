set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::获取第三方库
call "%bat%public.bat"

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::编译工程
call "%bat%scripts\rebuild_CMysqlAPI.py"

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::向公共部分提供文件
xcopy /y /i /r /s "%bat%CMysqlAPI\src\*.h" "%xueyelingshuang%include\CMysqlAPI\"
xcopy /y /i /r /s "%bat%CMysqlAPI\src\*.inl" "%xueyelingshuang%include\CMysqlAPI\"

xcopy /y /i /r /s "%bat%CMysqlAPI\props\*.props" "%xueyelingshuang%props\"