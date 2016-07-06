#pragma once
#include <afxwin.h>
#include "CtxtMacro.h"
#include <vector>
using namespace std;

class CtxtAPI Ctxt{
public:
	vector<vector<CString>> vectxt;

public:
	//txt导入分为两种，第一种，每行只取固定格子的东西，如"1-3,4-6"，第二种，每行以一个标志分隔，每行输入的最大长度默认一亿
	void LoadTxt(CString strPath,BOOL flag,CString strSplit);
};