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
	void LoadJson(CString strJson);

private:
	void LoadOneModule(CString strName,CString strValue,BOOL flag);
	void OnlyLoadOneJson(CString strJson);
};