#pragma once
#include <afxwin.h>
#include "CjsonMacro.h"

class CjsonAPI CFieldType{
public:
	CString field;
	int num;

	CFieldType();
	CFieldType(CString field,int num);
};