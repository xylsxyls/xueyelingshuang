#pragma once
#include <afxwin.h>
#include <string>
using namespace std;

class CAttri;
class CTable;
class CValue{
	//1是字符串，2是整数，3是小数，-1是运算表达式或者出错
public:
	BOOL Type;
	CString strValue;
	int nValue;
	double dValue;
	//转到表中的map字段
	//static CTable* pTable;
	CString strCurrentField;
	
public:
	CValue();
	CValue(CString strValue);
	CValue(char* szValue);
	CValue(string strValue);
	CValue(int nValue);
	CValue(double dValue);

	CValue(const CValue& value);
	CValue operator= (const CValue& value);
	bool operator== (const CValue& value);
	bool operator!= (const CValue& value);

	operator int();
	operator CString();
	operator double();

public:
	//如果要多线程操作的话需要先init才能使用CValue
	//void init(CTable* pTable);

public:
	//CAttri* operator->();
};