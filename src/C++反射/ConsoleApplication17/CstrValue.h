#pragma once
#include <afxwin.h>

class CstrValue{
public:
	BOOL type;
	CString strValue;
	int nValue;
	double dValue;

public:
	CstrValue();
	CstrValue(CString strValue);
	~CstrValue();
};