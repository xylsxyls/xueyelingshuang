#pragma once
#include <afxwin.h>

class CCondition{
public:
	CString strSQL;
	CCondition& operator&&(CCondition con);
	CCondition& operator||(CCondition con);
	CCondition& operator!();

	void Clear();
};