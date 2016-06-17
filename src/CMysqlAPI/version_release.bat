set bat=%~dp0
set xueyelingshuang=%bat%..\..\

call "%bat%public.bat"

call "%bat%scripts\rebuild_CMysqlAPI.py"

xcopy /y /i /r /s "%bat%CMysqlAPI\src\*.h" "%xueyelingshuang%include\CMysqlAPI\"
xcopy /y /i /r /s "%bat%CMysqlAPI\src\*.inl" "%xueyelingshuang%include\CMysqlAPI\"