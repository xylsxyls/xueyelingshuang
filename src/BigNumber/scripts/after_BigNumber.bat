::set bat=%~dp0
::set xueyelingshuang=%bat%..\..\..\

::del "%xueyelingshuang%include\gmp.h"
::del "%xueyelingshuang%lib\gmp.lib"
::del "%xueyelingshuang%lib\BigNumberBased.lib"
::del "%xueyelingshuang%lib\BigNumberBase.lib"

::rmdir /q /s "%xueyelingshuang%include\BigNumberBase\"