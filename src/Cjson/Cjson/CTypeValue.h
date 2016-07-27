#pragma once
#include <afxwin.h>
#include <typeinfo>
#include "Cjson.h"
#include "CszValue.h"
#include "CstrValue.h"


class CstrValue;
class CszValue;
class Cjson;

class CTypeValue{
public:
	CString type;
	void *pClass;

public:
	//存放json指针，用于寻找vecField和vecTypeValue
	Cjson *pJson;

public:
	CTypeValue();
	CTypeValue(const CstrValue& Class);
	CTypeValue(const CszValue& Class);
	CTypeValue(const Cjson& Class);

	CTypeValue(const CTypeValue& x);
	CTypeValue& operator = (const CTypeValue& x);
	CTypeValue& operator[] (CString field);
	CTypeValue& operator[] (int num);

	void TypeEqual(CString strTemp);

	CTypeValue& operator = (int nNum);
	CTypeValue& operator = (double dNum);
	CTypeValue& operator = (CString str);

public:
	CstrValue& toValue();
	CszValue& tosz();
	Cjson& toJson();
	~CTypeValue();
};