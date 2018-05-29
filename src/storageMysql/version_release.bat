set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::依赖CMysqlAPI
if exist "%xueyelingshuang%src\CMysqlAPI\version_release.bat" (
call "%xueyelingshuang%src\CMysqlAPI\version_release.bat"
) else (
echo src\CMysqlAPI\version_release.bat no exist
pause
exit
)

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::编译工程
call "%bat%scripts\rebuild_storage.py"

::call完之后都需要重新set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::删除临时文件
del "%bat%@AutomationLog.txt"
del "%bat%scripts\@AutomationLog.txt"
del "%bat%scripts\msbuild.log"
rmdir /q /s "%bat%storage\Release\"

::删除ilk，exp
del "%xueyelingshuang%lib\storage.ilk"
del "%xueyelingshuang%lib\storage.exp"