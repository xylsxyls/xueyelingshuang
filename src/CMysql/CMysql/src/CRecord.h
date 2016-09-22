#pragma once
#include <afxwin.h>
#include <map>
using namespace std;
#include "CValue.h"

class CTable;
class CRecord{
public:
	//不使用table中定义字段名的模式，因为无法添加新字段
	map<CString,CValue> mapValue;

	CString strCurrentField;
	CTable* pTable;
public:
	CRecord(CTable* pTable);
	CRecord(const CRecord& record);
	CRecord operator=(const CRecord& record);

public:
	//只能使用返回本身的方式，如果返回CValue则必须在CValue设置CTable的静态变量
	//CRecord& operator[](CString Field);
	CRecord& operator[](CString Field);
	//给记录赋值字段值
	CRecord operator=(const CValue& value);
	
	
	
public:
	CAttri* operator->();

public:
	CValue toValue();

public:
	CString ToCString();

};