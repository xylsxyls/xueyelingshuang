#pragma once
#include <map>
#include <vector>
using namespace std;
#include <afxwin.h>

class CstrValue;
class CszValue;
class CTypeValue;
class CFieldType;
class Cjson{
public:
	map<CString,CTypeValue> mapdata;

public:
	vector<CFieldType> vecField;
	vector<CTypeValue*> vecTypeValue;

public:
	

public:
	Cjson();
	~Cjson();

public:
	void LoadJson(CString strJson);
	vector<CString> GetField();
	int size();
	//FormatLength指的是里面的\t个数
	CString toCString(CString NewLineSign = "\r\n",CString FormatSign = "\t",int FormatLength = 1);
private:
	void LoadOneModule(CString strName,CString strValue,BOOL flag);
	void OnlyLoadOneJson(CString strJson);
	CString AddTypeValue(CString strResult,int *nInsert,BOOL ifFirst,CString strField,CTypeValue TypeValue,CString NewLineSign,CString FormatSign,int FormatLength);
};