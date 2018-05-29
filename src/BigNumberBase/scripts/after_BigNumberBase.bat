set bat=%~dp0
set xueyelingshuang=%bat%..\..\..\

del "%xueyelingshuang%include\gmp.h"
del "%xueyelingshuang%lib\libgcc.a"
del "%xueyelingshuang%lib\libgmp.a"
del "%xueyelingshuang%lib\libmingwex.a"
del "%xueyelingshuang%lib\libmsvcrt.a"