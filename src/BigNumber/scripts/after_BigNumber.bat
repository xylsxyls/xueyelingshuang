set bat=%~dp0
set xueyelingshuang=%bat%..\..\..\

del "%xueyelingshuang%lib\libgcc.a"
del "%xueyelingshuang%lib\libgmp.a"
del "%xueyelingshuang%lib\libmingwex.a"
del "%xueyelingshuang%lib\libmsvcrt.a"
del "%xueyelingshuang%lib\CStringManagerd.lib"
del "%xueyelingshuang%lib\BigNumberBased.lib"

rmdir /q /s "%xueyelingshuang%include\BigNumberBase\"
rmdir /q /s "%xueyelingshuang%include\CStringManager\"