#pragma once
#include <SDKDDKVer.h>
#include <afx.h>

class CValue{
	//1是字符串，2是整数，3是小数，-1是运算表达式或者出错
public:
	BOOL Type;
	CString strValue;
	int nValue;
	double dValue;
	
public:
	CValue();
	CValue(CString strValue);
	CValue(int nValue);
	CValue(double dValue);

	CValue(const CValue& value);
	CValue operator = (const CValue& value);
	bool operator== (const CValue& value);
	bool operator!= (const CValue& value);
};