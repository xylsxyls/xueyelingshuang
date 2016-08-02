#pragma once
#include <vector>
#include <map>
using namespace std;
#include <afxwin.h>
#include "CjsonMacro.h"

class Cjson;
class CjsonAPI CszValue{
public:
	vector<Cjson> vecszValue; //只存放两种，CstrValue和CjsonA

public:
	map<CString,CString> mapszError;

public:
	CszValue();
	CszValue(CString strValue);
	~CszValue();

public:
	int size();
};