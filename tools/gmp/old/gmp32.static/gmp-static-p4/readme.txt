如果在mingw32下使用：
复制.a到lib， 复制.h到include即可

如果在VC下使用：
把*.a文件加入lib搜索路径
把gmp.h文件加入include搜索路径
程序加入下面几行，即可
#include <gmp.h>

#pragma comment(lib, "libgmp.a")
#pragma comment(lib, "libgcc.a")
#pragma comment(lib, "libmingwex.a")
#pragma comment(lib, "libmsvcrt.a")
