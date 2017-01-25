#pragma once
#include <vector>
#include <map>
using namespace std;

#include "CjsonMacro.h"

class Cjson;
class CjsonAPI CszValue{
public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	vector<Cjson> vecszValue; //只存放两种，CstrValue和CjsonA
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	map<string, string> mapszError;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	

public:
	CszValue();
	CszValue(string strValue);
	~CszValue();

public:
	int size();
};