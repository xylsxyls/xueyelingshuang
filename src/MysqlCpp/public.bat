set bat=%~dp0
set xueyelingshuang=%bat%..\..\

xcopy /y /i /r /s "%xueyelingshuang%tools\mysqlcpp\include\*" "%xueyelingshuang%include\"

if "%1" == "32" (goto MysqlCpp_32) else (goto MysqlCpp_64)
:MysqlCpp_32
xcopy /y /i /r /s "%xueyelingshuang%tools\mysqlcpp\lib\release\*" "%xueyelingshuang%lib\"
goto MysqlCpp_end
:MysqlCpp_64
xcopy /y /i /r /s "%xueyelingshuang%tools\mysqlcpp\lib64\*" "%xueyelingshuang%lib\"
goto MysqlCpp_end
:MysqlCpp_end