#pragma once
#include <dlfcn.h>
#include <string>
#include <string.h>
#include <algorithm>
#include <functional>
#include <stdarg.h>
using namespace std;

typedef int MultiBool;
#define CLinuxDllLoad_FunNumber 100
#define CLinuxDllLoad_NameNumber 50
#define V(NewFunName) (void **)(&(NewFunName)) //调用时用的到，老版
#define V_str(NewFunName) FunName((void **)(&NewFunName),#NewFunName) //更新版

class FunName{
public:
	void **TempFunAddr;
	char strTempFun[CLinuxDllLoad_NameNumber];
	FunName(void **TempFunAddr,char* strTempFun);
};

class CLinuxDllLoad{
private:
	void *handle;
	void *FunArr[CLinuxDllLoad_FunNumber];

public:
	CLinuxDllLoad(string DllPath);
	~CLinuxDllLoad(void);

	int ErrorFunNumber;
	string ErrorFunName;

public:
	//老版，dll内的名字和自定义名字可以不同，需依次调用两个函数，外包用V，受CLOAD_FunNumber上限控制
	MultiBool Check_Fun(int number,...);//返回-1表示加载路径错误，返回1表示所有函数都存在，返回0表示有函数不存在
	void Load_Fun(int number,...);

public:
	//更新版，dll内的名字和自定义名字必须相同，外包用V_str，加载个数不受CLinuxDllLoad_FunNumber上限控制
	MultiBool LoadFun(int number,...);
	
};