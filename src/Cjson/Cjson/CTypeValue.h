#pragma once
#include <afxwin.h>
#include "CstrValue.h"
#include "CszValue.h"
#include "Cjson.h"

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

	//存放错误值
	CstrValue strValueError;
	CszValue szValueError;
	Cjson jsonError;

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