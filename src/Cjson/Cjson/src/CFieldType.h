#pragma once
#include <string>
using namespace std;
#include "CjsonMacro.h"

class CjsonAPI CFieldType{
public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	string field;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	
	int num;

	CFieldType();
	CFieldType(string field,int num);
};