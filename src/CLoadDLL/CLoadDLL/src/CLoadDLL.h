#pragma once
#include "CLoadDLLMacro.h"
#include <Windows.h>
#include <string>
using namespace std;

typedef int MultiBool;
#define V(NewFunName) (void **)(&(NewFunName)) //调用时用的到，老版
#define CLOAD_FunNumber 200

#define V_str(NewFunName) FunName((void **)(&NewFunName),#NewFunName) //更新版

class CLoadDLLAPI FunName{
public:
	void** TempFunAddr;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	string strTempFun;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	FunName(void **TempFunAddr,const char szTempFun[]);
};

class CLoadDLLAPI CLoadDLL
{
private:
	HINSTANCE FPI_DllHinstance;
	void *FunArr[CLOAD_FunNumber];

public:
	CLoadDLL(string DllPath);
	~CLoadDLL(void);

	int ErrorFunNumber;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	string ErrorFunName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	
public:
	//老版，dll内的名字和自定义名字可以不同，需依次调用两个函数，外包用V，受CLOAD_FunNumber上限控制
	MultiBool Check_Fun(int number,...); //返回-1表示加载路径错误，返回1表示所有函数都存在，返回0表示有函数不存在
	void Load_Fun(int number,...);

public:
	//更新版，dll内的名字和自定义名字必须相同，外包用V_str，加载个数不受CLOAD_FunNumber上限控制
	MultiBool LoadFun(int number,...);
};