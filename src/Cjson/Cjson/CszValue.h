#pragma once
#include <vector>
using namespace std;
#include <afxwin.h>
#include "Cjson.h"
#include "CszOneValue.h"
#include "CTypeValue.h"

class Cjson;
class CTypeValue;
class CszOneValue;
class CszValue{
public:
	vector<CszOneValue> vecszOneValue;
	vector<Cjson> vecjson;

	vector<CTypeValue> vecszValue; //只存放两种，CstrValue和Cjson

public:
	CszValue();
	CszValue(CString strValue);
	~CszValue();
};