#pragma once
#include <afxwin.h>
#include <map>
using namespace std;
#include "CValue.h"

class CRecord{
public:
	//不使用table中定义字段名的模式，因为无法添加新字段
	map<CString,CValue> mapValue;

	CString strCurrentField;
public:
	CRecord();
	CRecord(const CRecord& record);
	CRecord operator=(const CRecord& record);

public:
	CValue& operator[](CString Field);

public:
	CString ToCString();
};