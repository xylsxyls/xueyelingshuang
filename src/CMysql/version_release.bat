::release-------------------------------------------------------------------------

set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::�����Ҫ��ȡ����dll-----------------------------------------------------------
call "%xueyelingshuang%src\..relyproj..\version_release.bat"

::call��֮����Ҫ����set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\
::�����Ҫ��ȡ����dll-----------------------------------------------------------

call "%xueyelingshuang%src\CStringManager\version_release.bat"

::call��֮����Ҫ����set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\
::�����Ҫ��ȡ����dll-----------------------------------------------------------

::��ȡ��������
call "%bat%public.bat"

::call��֮����Ҫ����set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::���빤��
call "%bat%scripts\rebuild_CMysql.py"

::call��֮����Ҫ����set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::�򹫹������ṩ�ļ����е�ʱ�����û��inl�ļ�
xcopy /y /i /r /s "%bat%CMysql\src\*.h"   "%xueyelingshuang%include\CMysql\"
xcopy /y /i /r /s "%bat%CMysql\src\*.inl" "%xueyelingshuang%include\CMysql\"
xcopy /y /i /r /s "%bat%CMysql\props\*.props" "%xueyelingshuang%props\"

::ɾ����ʱ�ļ�
del "%bat%@AutomationLog.txt"
del "%bat%scripts\@AutomationLog.txt"
del "%bat%scripts\msbuild.log"
rmdir /q /s "%bat%CMysql\vcxproj\Release\"

::ɾ��ilk��exp
del "%xueyelingshuang%lib\CMysql.ilk"
del "%xueyelingshuang%lib\CMysql.exp"