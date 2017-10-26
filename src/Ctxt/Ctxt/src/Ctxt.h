#pragma once
#include "CtxtMacro.h"
#include <vector>
#include <stdint.h>
#include <map>

class CtxtAPI Ctxt
{
public:
	enum
	{
		POINT_TO_POINT = 1,
		SPLIT,
		ONE_LINE
	};

public:
	Ctxt(const std::string& strPath);
	~Ctxt();

public:
	//?内部使用ifstream和getline，文件末尾的多余空行不会被加载，txt导入分为两种，第一种，每行只取固定格子的东西，如"1-3,4-6"，顶格就是1，1-3就是取123格，第二种，每行以一个标志分隔，每行输入的最大长度默认一亿，第三种，一行一加载
	void LoadTxt(int32_t flag, const std::string& strSplit);
	//?转化为map形式，通过[string][/string]方式找到开始行和结束行
	void ToMap();
	//?把内存中的保存到文件里
	void Save();
	//?以重写方式打开一个文件
	bool OpenFile_w();
	//?以追加方式打开一个文件
	bool OpenFile_a();
	//?追加写入一行
	void AddWriteLine(const char* fmt, ...);
	//?关闭文件
	void CloseFile();
	//?追加写入一行不打开文件
	void AddLine(const char* fmt, ...);
	//?另存
	void SaveAs(const std::string& path);

private:
	//?用点到点的方式加载
	void LoadTxtWithPointToPoint(const std::string& strSplit);
	//?用分隔的方式加载
	void LoadTxtWithSplit(const std::string& strSplit);
	//?整行加载
	void LoadTxtWithOneLine();

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::vector<std::vector<std::string>> m_vectxt;
	//?转化的map形式
	std::map<std::string, std::vector<int32_t>> m_maptxt;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	FILE *m_pFile;
	std::string m_strPath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	void* m_txt;
};