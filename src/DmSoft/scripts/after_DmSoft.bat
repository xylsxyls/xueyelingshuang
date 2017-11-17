set bat=%~dp0
set xueyelingshuang=%bat%..\..\..\

del "%xueyelingshuang%include\DmSoft\Cdmsoft.h"
xcopy /y /i /r /s "%xueyelingshuang%tools\´óÄ®²å¼þ\dm.dll" "%xueyelingshuang%lib\"