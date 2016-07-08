#pragma once
#include <afxwin.h>
#include "CtxtMacro.h"
#include <vector>
using namespace std;

class CtxtAPI Ctxt{
public:
	vector<vector<CString>> vectxt;
	FILE *pFile;
	CString strPath;

public:
	Ctxt(CString strPath);

public:
	//内部使用ifstream直接调用即可，txt导入分为两种，第一种，每行只取固定格子的东西，如"1-3,4-6"，顶格就是1，1-3就是取123格，第二种，每行以一个标志分隔，每行输入的最大长度默认一亿
	void LoadTxt(BOOL flag,CString strSplit);
	//以追加方式打开一个文件
	bool OpenFile_a();
	//追加写入一行
	void AddWriteLine(int MaxLength,CString format,...);
	//关闭文件
	void CloseFile();
};