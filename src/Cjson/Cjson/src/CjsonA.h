#pragma once
#include <unordered_map>
#include <map>
#include <vector>
#include <string>
using namespace std;
#include <afxwin.h>
#include "CjsonMacro.h"
#include "CFieldType.h"

class CstrValue;
class CszValue;
class Cjson;
class CFieldType;

class CjsonAPI CjsonA{
public:
	unordered_map<string,Cjson> mapdata;

public:
	//给new出来的空间用，这里一直为空
	vector<CFieldType> vecField;
	//一定需要把所有层全部存下，因为会出现用户改变当前层属性的情况，全部存下才能找到出错前的上一层
	vector<Cjson*> vecCjson;

public:
	//第一个是不可识别的字符串，第二个是说明
	map<CString,CString> mapError;

public:
	CjsonA();
	~CjsonA();

	CjsonA(const CjsonA& json);
	CjsonA operator = (const CjsonA& json);

public:
	map<CString,CString> LoadJson(CString strJson);
	vector<CString> GetField();
	int size();
	//FormatLength指的是里面的\t个数
	CString toCString(CString NewLineSign = "\r\n",CString FormatSign = "\t",int FormatLength = 1);
private:
	BOOL GetOneModule(CString* strJson,BOOL* flag,CString* strName,CString *strValue);
	void GetOneJson(CString *strJson,CString* strOneJson);
	void LoadOneModule(CString strName,CString strValue,BOOL flag);
	void OnlyLoadOneJson(CString strJson);
	CString AddTypeValue(CString strResult,int *nInsert,BOOL ifFirst,CString strField,Cjson TypeValue,CString NewLineSign,CString FormatSign,int FormatLength);
};