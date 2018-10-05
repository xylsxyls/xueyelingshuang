#pragma once
#include "CDumpMacro.h"
#include <string>

class CDumpAPI CDump
{
private:
    CDump();

public:
    static CDump& instance();

public:
	//执行这句之后程序之后如果再出错就会在当前进程下产生dump文件，建议传绝对路径
	void DeclareDumpFile(const std::string& dumpFilePath = "");

    std::string GetDumpFilePath();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::string m_dumpFilePath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};