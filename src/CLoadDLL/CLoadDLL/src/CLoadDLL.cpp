#include "CLoadDLL.h"
#include "CStringManager/CStringManagerAPI.h"

CLoadDLL::CLoadDLL(string DllPath)
{
	FPI_DllHinstance = LoadLibraryEx(DllPath.c_str(),0,LOAD_WITH_ALTERED_SEARCH_PATH); //¶¯Ì¬¼ÓÔØDLL
	memset(FunArr,0,CLOAD_FunNumber);
	string ErrorFunName = "";
	ErrorFunNumber = 0;
}

CLoadDLL::~CLoadDLL()
{
	::FreeLibrary(FPI_DllHinstance); //ÊÍ·ÅDLL
}

MultiBool CLoadDLL::Check_Fun(int number,...){
	if(FPI_DllHinstance == NULL) return -1;
	va_list list;
	va_start(list,number);
	int n = 0;
	int AllRight = 1;
	while(n != number){
		char *TempFun = va_arg(list,char *);
		FunArr[n++] = (void *)(void(__stdcall *)())GetProcAddress(FPI_DllHinstance,TempFun);
		if(FunArr[n - 1] == 0){
			ErrorFunName = ErrorFunName + TempFun + " ";
			ErrorFunNumber++;
			AllRight = 0;
		}
	}
	va_end(list);
	return AllRight;
}

void CLoadDLL::Load_Fun(int number,...){
	va_list list;
	va_start(list,number);
	int n = 0;
	while(n != number){
		*(va_arg(list,void **)) = FunArr[n++];
	}
	va_end(list);
	return ;
}

FunName::FunName(void **TempFunAddr,const char szTempFun[]){
	this->TempFunAddr = TempFunAddr;
	this->strTempFun = szTempFun;

	CStringManager str = strTempFun;
	str.ReplaceEvery("(","");
	str.ReplaceEvery(")","");
	str.ReplaceEvery(" ","");
	this->strTempFun = str.strInside;
	//strTempFun.Replace("(","");
	//strTempFun.Replace(")","");
	//strTempFun.Replace(" ","");
};

MultiBool CLoadDLL::LoadFun(int number,...){
	if(FPI_DllHinstance == NULL) return -1;
	va_list list;
	va_start(list,number);
	int AllRight = 1;
	while(number-- != 0){
		FunName AddrAndName = (va_arg(list,FunName));
		*AddrAndName.TempFunAddr = (void *)(void(__stdcall *)())GetProcAddress(FPI_DllHinstance,AddrAndName.strTempFun.c_str());
		if(*AddrAndName.TempFunAddr == 0){
			ErrorFunName = ErrorFunName + AddrAndName.strTempFun + " ";
			ErrorFunNumber++;
			AllRight = 0;
		}
	}
	va_end(list);
	return AllRight;
}