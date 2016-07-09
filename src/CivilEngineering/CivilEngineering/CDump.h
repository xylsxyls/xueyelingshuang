#pragma once
#include <afxwin.h>
#include "CDumpMacro.h"
#pragma comment(lib,"Dbghelp.lib")

class CDumpAPI CDump{
public:
	//执行这句之后程序之后如果再出错就会在当前进程下产生dump文件，建议传绝对路径
	void DeclareDumpFile(CString DumpFilePath = "CrashDumpFile.dmp");
};