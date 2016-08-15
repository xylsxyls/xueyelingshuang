#pragma once
#include <afxwin.h>
#include "CstrValue.h"
#include "CszValue.h"
#include "CjsonA.h"
#include "CjsonMacro.h"

class CstrValue;
class CszValue;
class CjsonA;

class CjsonAPI Cjson{
public:
	CString type;
	void *pClass;

	//这个代表的是现在这个CTypeValue是不是使用主体
	CjsonA *pJsonA;

public:
	//存放json指针，用于寻找vecField和vecCjson，必须要有一个透传值来决定下层CTypeValue是否需要构造主体
	//pJson一路透传来决定pJsonA是否需要构造，但是访问最外层直接在进入函数时通过pJson拿到最外层pJson，这层的pJson一定不会因为insert，delete等原因导致丢失
	CjsonA *pJson;

	//存放错误值
	CstrValue strValueError;
	CszValue szValueError;
	CjsonA jsonError;

public:
	map<CString,CString> LoadJson(CString strJson);
	vector<CString> GetField();
	int size();
	//FormatLength指的是里面的\t个数
	CString toCString(CString NewLineSign = "\r\n",CString FormatSign = "\t",int FormatLength = 1);

public:
	Cjson();
	Cjson(const CstrValue& Class);
	Cjson(const CszValue& Class);
	Cjson(const CjsonA& Class);

	Cjson(const Cjson& x);
	Cjson operator = (const Cjson& x);
	Cjson& operator[] (CString field);
	Cjson& operator[] (int num);

	//返回最外层指针用于等于的传值
	Cjson* TypeEqual(CString strTemp);

	Cjson operator = (int nNum);
	Cjson operator = (double dNum);
	Cjson operator = (CString str);

public:
	CstrValue& toValue();
	CszValue& tosz();
	CjsonA& toJson();

public:
	vector<HWND> vecHwnd;
	void ClearHwnd();
	void SaveHwnd(HWND hwnd);
	HWND GetHwnd(int num);

	void FreeCJson();
	~Cjson();
};