#pragma once
#include "CDumpMacro.h"
#pragma comment(lib,"Dbghelp.lib")
#include <string>
using namespace std;

class CDumpAPI CDump{
public:
	//执行这句之后程序之后如果再出错就会在当前进程下产生dump文件，建议传绝对路径
	void DeclareDumpFile(string DumpFilePath = "CrashDumpFile.dmp");
};