set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::�����Ҫ��ȡ����dll-----------------------------------------------------------
call "%xueyelingshuang%src\MulPoint\version_debug.bat"

::call��֮����Ҫ����set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\
::�����Ҫ��ȡ����dll-----------------------------------------------------------

::���빤��
call "%bat%scripts\rebuild_MulRect.py" debug

::call��֮����Ҫ����set
set bat=%~dp0
set xueyelingshuang=%bat%..\..\

::�򹫹������ṩ�ļ����е�ʱ�����û��inl�ļ�
xcopy /y /i /r /s "%bat%MulRect\src\*.h"   "%xueyelingshuang%include\MulRect\"
xcopy /y /i /r /s "%bat%MulRect\src\*.inl" "%xueyelingshuang%include\MulRect\"

::ɾ����ʱ�ļ�
del "%bat%@AutomationLog.txt"
del "%bat%scripts\@AutomationLog.txt"
del "%bat%scripts\msbuild.log"
rmdir /q /s "%bat%MulRect\vcxproj\Debug\"

::ɾ��ilk��exp
del "%xueyelingshuang%lib\MulRectd.ilk"
del "%xueyelingshuang%lib\MulRectd.exp"