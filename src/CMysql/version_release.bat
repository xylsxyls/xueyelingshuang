::release-------------------------------------------------------------------------

set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::�����Ҫ��ȡ����dll-----------------------------------------------------------
call %xueyelingshuang%src\..relyproj..\version_release.bat

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