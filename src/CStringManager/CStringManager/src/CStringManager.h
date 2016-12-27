#pragma once
#include <afxwin.h>
#include <vector>
#include "CStringManagerMacro.h"
using namespace std;

class CStringManagerAPI CStringManager{
public:
	CString str;

public:
	CStringManager(CString str);
	CStringManager operator = (CString str);

public:
	//从0开始计数，根据当前选择返回另一侧符号的位置，如果给定左右两侧不存在或选择的位置不是两侧之一则返回-1
	//两侧个数不匹配的时候会根据左侧开始返回对应的右侧
	int FindOther(char cLeft,char cRight,int nSelect);
	//分割字符串
	vector<CString> split(CString separate_character);
	//vector<CString> vec = split("123,234,345,456,",","); //得到5个字符串
	//vector<CString> vec2 = split("123,,234,,345,,456",",,"); //得到4个字符串

	string Randomstring(int nMin,int nMax,vector<char> vecCharacter);
};