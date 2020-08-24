#pragma once
#include "CtxtMacro.h"
#include <string>
#include <vector>
#include <stdint.h>

class CtxtAPI Ctxt
{
public:
	//以\r\n为行结束符
	enum Load
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
	void LoadTxt(Load flag, const std::string& strSplit = "");
	//?把内存中的保存到文件里
	bool Save();
	//?另存
	bool SaveAs(const std::string& path);
	//?文件内群体换
	int32_t Replace(const std::string& oldstr, const std::string& newstr);
	//?追加写入一行不打开文件，含有换行符
	void AddLineWithoutOpenFile(const char* fmt, ...);

private:
	//?用点到点的方式加载
	void LoadTxtWithPointToPoint(const std::string& strSplit);
	//?用分隔的方式加载
	void LoadTxtWithSplit(const std::string& strSplit);
	//?整行加载
	void LoadTxtWithOneLine();
	//?写入，不含有换行符
	void Write(FILE* pFile, const char* fmt, ...);

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::vector<std::vector<std::string>> m_vectxt;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_strPath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	void* m_txt;
};