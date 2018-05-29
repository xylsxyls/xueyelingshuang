set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::依赖CMysqlAPI
if exist "%xueyelingshuang%src\CMysqlAPI\version_debug.bat" (
call "%xueyelingshuang%src\CMysqlAPI\version_debug.bat"
) else (
echo src\CMysqlAPI\version_debug.bat no exist
pause
exit
)

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::编译工程
call "%bat%scripts\rebuild_storage.py" debug

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::删除临时文件
del "%bat%@AutomationLog.txt"
del "%bat%scripts\@AutomationLog.txt"
del "%bat%scripts\msbuild.log"
rmdir /q /s "%bat%storage\Debug\"

::删除ilk，exp
del "%xueyelingshuang%lib\storaged.ilk"
del "%xueyelingshuang%lib\storaged.exp"