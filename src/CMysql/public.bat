set bat=%~dp0
set xueyelingshuang=%bat%..\..\

xcopy /y /i /r /s "%xueyelingshuang%tools\MySql_lib_dll\32\libmysql.lib"  "%xueyelingshuang%lib\"
xcopy /y /i /r /s "%xueyelingshuang%tools\MySql_lib_dll\32\libmysql.dll"  "%xueyelingshuang%lib\"

xcopy /y /i /r /s "%xueyelingshuang%tools\MySql_lib_dll\mysql.h"  "%MYSQL_ROOT%\include\"
