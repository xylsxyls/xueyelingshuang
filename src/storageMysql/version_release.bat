set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::����CMysqlAPI
if exist "%xueyelingshuang%src\CMysqlAPI\version_release.bat" (
call "%xueyelingshuang%src\CMysqlAPI\version_release.bat"
) else (
echo src\CMysqlAPI\version_release.bat no exist
pause
exit
)

::call��֮����Ҫ����set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::���빤��
call "%bat%scripts\rebuild_storage.py"

::call��֮����Ҫ����set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::ɾ����ʱ�ļ�
del "%bat%@AutomationLog.txt"
del "%bat%scripts\@AutomationLog.txt"
del "%bat%scripts\msbuild.log"
rmdir /q /s "%bat%storage\Release\"

::ɾ��ilk��exp
del "%xueyelingshuang%lib\storage.ilk"
del "%xueyelingshuang%lib\storage.exp"