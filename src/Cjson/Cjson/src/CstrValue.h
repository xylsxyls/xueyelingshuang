#pragma once

#include "CjsonMacro.h"
typedef int BOOL;
#include <string>
using namespace std;

class CjsonAPI CstrValue{
	//1是字符串，2是整数，3是小数，-1是运算表达式或者出错
public:
	BOOL type;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	string strValue;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	
	int nValue;
	double dValue;

public:
	CstrValue();
	CstrValue(BOOL type,string strValue,int nValue,double dValue);
	CstrValue(string strValue);
	CstrValue(const CstrValue& x);
	CstrValue operator= (const CstrValue& x);

public:
	operator int();
	operator string();
	operator double();

public:
	~CstrValue();
};