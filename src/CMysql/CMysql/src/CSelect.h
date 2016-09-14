#pragma once
#include <SDKDDKVer.h>
#include <afx.h>
#include <list>
#include "CTableField.h"
using namespace std;

class CSelect{
public:
	list<CTableField> list;
	CTableField tf;

public:
	//sel("User")["ID"];
	CSelect operator()(CString Table);
	CSelect operator[](CString Field);

public:
	CSelect();
	CSelect(const CSelect& sel);
	CSelect operator=(const CSelect& sel);
};