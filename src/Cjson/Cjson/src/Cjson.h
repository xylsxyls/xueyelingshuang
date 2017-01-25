#pragma once
#include "CstrValue.h"
#include "CszValue.h"
#include "CjsonA.h"
#include "CjsonMacro.h"

class CstrValue;
class CszValue;
class CjsonA;

class CjsonAPI Cjson{
public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	string type;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	
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
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	map<string, string> LoadJson(string strJson);
	vector<string> GetField();
	int size();
	//FormatLength指的是里面的\t个数
	string tostring(string NewLineSign = "\r\n", string FormatSign = "\t", int FormatLength = 1);
#ifdef _MSC_VER
#pragma warning(pop)
#endif

public:
	Cjson();
	Cjson(const CstrValue& Class);
	Cjson(const CszValue& Class);
	Cjson(const CjsonA& Class);

	Cjson(const Cjson& x);
	Cjson operator = (const Cjson& x);
	Cjson& operator[] (string field);
	Cjson& operator[] (int num);

	//返回最外层指针用于等于的传值
	Cjson* TypeEqual(string strTemp);

	Cjson operator = (int nNum);
	Cjson operator = (double dNum);
	Cjson operator = (string str);

public:
	CstrValue& toValue();
	CszValue& tosz();
	CjsonA& toJson();

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//vector<HWND> vecHwnd;
	//void ClearHwnd();
	//void SaveHwnd(HWND hwnd);
	//HWND GetHwnd(int num);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	

	void FreeCJson();
	~Cjson();
};