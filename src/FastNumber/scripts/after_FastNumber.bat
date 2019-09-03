set bat=%~dp0
set xueyelingshuang=%bat%..\..\..\

rd /s /q "%xueyelingshuang%include\BigNumber"
xcopy /y /i /r /s "%xueyelingshuang%include\FastNumber\*" "%xueyelingshuang%include\BigNumber\"
rd /s /q "%xueyelingshuang%include\FastNumber"