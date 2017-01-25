#pragma once
#include <unordered_map>
#include <map>
#include <vector>
#include <string>
using namespace std;

#include "CjsonMacro.h"
#include "CFieldType.h"

class CstrValue;
class CszValue;
class Cjson;
class CFieldType;

typedef int BOOL;

class CjsonAPI CjsonA{
public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	unordered_map<string, Cjson> mapdata;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//给new出来的空间用，这里一直为空
	vector<CFieldType> vecField;
	//一定需要把所有层全部存下，因为会出现用户改变当前层属性的情况，全部存下才能找到出错前的上一层
	vector<Cjson*> vecCjson;

public:
	//第一个是不可识别的字符串，第二个是说明
	map<string, string> mapError;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	

public:
	CjsonA();
	~CjsonA();

	CjsonA(const CjsonA& json);
	CjsonA operator = (const CjsonA& json);

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	map<string, string> LoadJson(string strJson);
	vector<string> GetField();
	int size();
	//FormatLength指的是里面的\t个数
	string tostring(string NewLineSign = "\r\n", string FormatSign = "\t", int FormatLength = 1);
private:
	BOOL GetOneModule(string* strJson, BOOL* flag, string* strName, string *strValue);
	void GetOneJson(string *strJson, string* strOneJson);
	void LoadOneModule(string strName, string strValue, BOOL flag);
	void OnlyLoadOneJson(string strJson);
	string AddTypeValue(string strResult, int *nInsert, BOOL ifFirst, string strField, Cjson TypeValue, string NewLineSign, string FormatSign, int FormatLength);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	
};