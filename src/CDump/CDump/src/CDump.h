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
	//ִ�����֮�����֮������ٳ���ͻ��ڵ�ǰ�����²���dump�ļ������鴫����·��
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