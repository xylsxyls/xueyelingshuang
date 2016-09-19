#pragma once
#include <afxwin.h>
#include "CValue.h"
#include "CRecord.h"

class CRecord;
class CAttri{
public:
	bool Type;
	CString Name;
	int nlength;
	CValue vDefault;
	bool bPrimaryKey;
	bool bNotNull;
	int nAutoIncrement;
	bool bZeroFill;
	CString strComment;
public:
	CAttri();
	CAttri(const CAttri& attri);
	CAttri operator= (const CAttri& attri);
};