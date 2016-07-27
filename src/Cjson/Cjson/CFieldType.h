#pragma once
#include <afxwin.h>

class CFieldType{
public:
	CString field;
	int num;

	CFieldType();
	CFieldType(CString field,int num);
};