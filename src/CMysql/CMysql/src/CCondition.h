#pragma once
#include <SDKDDKVer.h>
#include <afx.h>


class CCondition{
public:
	CString strSQL;
	CCondition operator&&(CCondition con);
	CCondition operator||(CCondition con);
	CCondition operator!();
};