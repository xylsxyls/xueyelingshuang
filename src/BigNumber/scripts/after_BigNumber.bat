set bat=%~dp0
set xueyelingshuang=%bat%..\..\..\

del "%xueyelingshuang%lib\libgcc.a"
del "%xueyelingshuang%lib\libgmp.a"
del "%xueyelingshuang%lib\libmingwex.a"
del "%xueyelingshuang%lib\libmsvcrt.a"
del "%xueyelingshuang%lib\BigNumberBased.lib"
del "%xueyelingshuang%lib\BigNumberBase.lib"

rmdir /q /s "%xueyelingshuang%include\BigNumberBase\"