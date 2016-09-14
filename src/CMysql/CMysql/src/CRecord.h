#pragma once
#include <SDKDDKVer.h>
#include <afx.h>
#include <map>
using namespace std;
#include "CValue.h"
#include "CAttri.h"

class CAttri;
class CRecord{
public:
	map<CString,CValue> mapValue;
	map<CString,CAttri> mapAttri;
	CString strCurrentField;
public:
	CRecord();
	CRecord(const CRecord& record);
	CRecord operator=(const CRecord& record);

public:
	CAttri& operator[](CString Field);
	CAttri operator=(const CAttri& attri);
};