#pragma once
#include <afxwin.h>
#include "CStringManagerMacro.h"

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
};