set bat=%~dp0
set xueyelingshuang=%bat%..\..\

xcopy /y /i /r /s "%xueyelingshuang%tools\mysqlcpp\include\*" "%xueyelingshuang%include\"
xcopy /y /i /r /s "%xueyelingshuang%tools\mysqlcpp\lib\%1\*" "%xueyelingshuang%lib\"