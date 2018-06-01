set bat=%~dp0
set xueyelingshuang=%bat%..\..\..\

del "%xueyelingshuang%include\mysql_connection.h"
del "%xueyelingshuang%include\mysql_driver.h"
del "%xueyelingshuang%include\mysql_error.h"
del "%xueyelingshuang%lib\mysqlcppconn.lib"
del "%xueyelingshuang%lib\mysqlcppconn-static.lib"
del "%xueyelingshuang%lib\CStringManagerd.lib"
del "%xueyelingshuang%lib\CStringManager.lib"

rmdir /q /s "%xueyelingshuang%include\cppconn\"
rmdir /q /s "%xueyelingshuang%include\CStringManager\"