#pragma once
#include <vector>
#include <map>
using namespace std;
#include <afxwin.h>

class CTypeValue;
class CszValue{
public:
	vector<CTypeValue> vecszValue; //只存放两种，CstrValue和Cjson

public:
	map<CString,CString> mapszError;

public:
	CszValue();
	CszValue(CString strValue);
	~CszValue();

public:
	int size();
};