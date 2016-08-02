#pragma once
#include <map>
#include <vector>
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
	map<CString,Cjson> mapdata;

public:
	//给new出来的空间用，这里一直为空
	vector<CFieldType> vecField;
	vector<Cjson*> veCjson;

public:
	//第一个是不可识别的字符串，第二个是说明
	map<CString,CString> mapError;

public:
	CjsonA();
	~CjsonA();

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