#pragma once
#include "CtxtMacro.h"
#include <vector>
using namespace std;

class CtxtAPI Ctxt{
public:

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	vector<vector<string>> vectxt;
	FILE *pFile;
	string strPath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

public:
	Ctxt(string strPath);
	~Ctxt();

public:
	//?内部使用ifstream和getline，文件末尾的多余空行不会被加载，txt导入分为两种，第一种，每行只取固定格子的东西，如"1-3,4-6"，顶格就是1，1-3就是取123格，第二种，每行以一个标志分隔，每行输入的最大长度默认一亿
	void LoadTxt(int flag,string strSplit);
	//?把内存中的保存到文件里
	void Save();
	//?以重写方式打开一个文件
	bool OpenFile_w();
	//?以追加方式打开一个文件
	bool OpenFile_a();
	//?追加写入一行
	void AddWriteLine(string format,...);
	//?关闭文件
	void CloseFile();
	//?追加写入一行不打开文件
	void AddLine(string format,...);
};