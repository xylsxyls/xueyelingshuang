set bat=%~dp0
set xueyelingshuang=%bat%..\..\

xcopy /y /i /r /s "%xueyelingshuang%tools\mysqlcpp\include\*" "%xueyelingshuang%include\"

if "%1" == "32" (goto MysqlCpp_32) else (goto MysqlCpp_64)
:MysqlCpp_32
if "%2" == "release" (goto MysqlCpp_release) else (goto MysqlCpp_debug)
:MysqlCpp_release
xcopy /y /i /r /s "%xueyelingshuang%tools\mysqlcpp\lib\release\*" "%xueyelingshuang%lib\"
goto MysqlCpp_end
:MysqlCpp_64
xcopy /y /i /r /s "%xueyelingshuang%tools\mysqlcpp\lib64\*" "%xueyelingshuang%lib\"
goto MysqlCpp_end
:MysqlCpp_end
goto MysqlCpp_debug_release_end
:MysqlCpp_debug
xcopy /y /i /r /s "%xueyelingshuang%tools\mysqlcpp\lib\debug\*" "%xueyelingshuang%lib\"
goto MysqlCpp_debug_release_end
:MysqlCpp_debug_release_end