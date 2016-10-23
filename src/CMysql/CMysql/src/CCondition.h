#pragma once
#include <afxwin.h>

class CCondition{
public:
	CString strSQL;
	//选择显示行数从0开始，看中间的间隔，0和1的间隔表示第一行，选择前4行应该是0到4
	int nLineBegin;
	int nLineEnd;
	CString strOrderField;
	CString strAscDesc;

	CCondition();
	CCondition& operator&&(CCondition con);
	CCondition& operator||(CCondition con);
	CCondition& operator!();
	CCondition& operator()(int nLineBegin,int nLineEnd);
	CCondition& operator[](CString strOrderField);
	const CCondition& operator++(int);
	const CCondition& operator--(int);

	CString toCString();
	void Clear();
};