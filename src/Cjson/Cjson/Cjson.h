#pragma once
#include <vector>
using namespace std;
#include <afxwin.h>
#include <map>
#include "CstrValue.h"
#include "CszValue.h"
#include "CTypeValue.h"

class CszValue;
class CTypeValue;
class Cjson{
public:
	//json中有3中形式，字段接字段值，字段接数组值，字段接json
	vector<CString> vecstr ;
	vector<CString> vecsz  ;
	vector<CString> vecjson;
	//通过map存储实际的值
	map<CString,CstrValue> mapstr ; //存储字段接字段值
	map<CString,CszValue > mapsz  ; //存储字段接数组值
	map<CString,Cjson    > mapjson; //存储字段接json

	map<CString,CTypeValue> mapdata;

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