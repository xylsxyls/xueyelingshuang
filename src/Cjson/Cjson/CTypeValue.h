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
	CTypeValue();
	CTypeValue(const CstrValue& Class);
	CTypeValue(const CszValue& Class);
	CTypeValue(const Cjson& Class);

	CTypeValue(const CTypeValue& x);
	CTypeValue operator = (const CTypeValue& x);
	~CTypeValue();
};