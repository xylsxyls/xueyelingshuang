#pragma once
#include <afxwin.h>
#include "CjsonMacro.h"

class CjsonAPI CstrValue{
	//1是字符串，2是整数，3是小数，-1是运算表达式或者出错
public:
	BOOL type;
	CString strValue;
	int nValue;
	double dValue;

public:
	CstrValue();
	CstrValue(BOOL type,CString strValue,int nValue,double dValue);
	CstrValue(CString strValue);
	CstrValue(const CstrValue& x);
	CstrValue operator= (const CstrValue& x);

public:
	operator int();
	operator CString();
	operator double();

public:
	~CstrValue();
};