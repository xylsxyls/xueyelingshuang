set bat=%~dp0
set xueyelingshuang=%bat%..\..\..\

xcopy /y /i /r /s "%bat%..\DialogManager\src\res\*" "%xueyelingshuang%lib\res\"