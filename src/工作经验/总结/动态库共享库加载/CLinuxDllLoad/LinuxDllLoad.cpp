#include "LinuxDllLoad.h"

//把x后的字符串全部向前移动一格
void FrontMove(char *str,int x){
	while(1){
		str[x] = str[x + 1];
		x++;
		if(str[x + 1] == 0){
			str[x] = 0;
			return;
		}
	}
}

void strRemoveAll(char *str,char c)
{
	int x = 0;
	while(*str + x){
		if(str[x] == c){
			FrontMove(str,x);
			continue;
		}
		x++;
	}
	return;
}

FunName::FunName(void **TempFunAddr,char* strTempFun){
	this->TempFunAddr = TempFunAddr;
	strRemoveAll(strTempFun,'(');
	strRemoveAll(strTempFun,')');
	strRemoveAll(strTempFun,' ');
	strcpy(this->strTempFun,strTempFun);
};

CLinuxDllLoad::CLinuxDllLoad(string DllPath)
{
	handle = dlopen(DllPath.c_str(),RTLD_NOW);
	memset(FunArr,0,sizeof(FunArr));
	string ErrorFunName = "";
	ErrorFunNumber = 0;
}

CLinuxDllLoad::~CLinuxDllLoad()
{
	if(handle != 0) dlclose(handle); //释放DLL
}

MultiBool CLinuxDllLoad::Check_Fun(int number,...){
	if(handle == NULL) return -1;
	va_list list;
	va_start(list,number);
	int n = 0;
	int AllRight = 1;
	while(n != number){
		char *TempFun = va_arg(list,char *);
		FunArr[n++] = (void *)dlsym(handle,TempFun);
		if(FunArr[n - 1] == 0){
			ErrorFunName = ErrorFunName + TempFun + " ";
			ErrorFunNumber++;
			AllRight = 0;
		}
	}
	va_end(list);
	return AllRight;
}

void CLinuxDllLoad::Load_Fun(int number,...){
	va_list list;
	va_start(list,number);
	int n = 0;
	while(n != number){
		*(va_arg(list,void **)) = FunArr[n++];
	}
	va_end(list);
	return ;
}

MultiBool CLinuxDllLoad::LoadFun(int number,...){
	if(handle == NULL) return -1;
	va_list list;
	va_start(list,number);
	int AllRight = 1;
	while(number-- != 0){
		FunName AddrAndName = (va_arg(list,FunName));
		*(AddrAndName.TempFunAddr) = (void *)dlsym(handle,AddrAndName.strTempFun);
		if(*AddrAndName.TempFunAddr == 0){
			ErrorFunName = ErrorFunName + AddrAndName.strTempFun + " ";
			ErrorFunNumber++;
			AllRight = 0;
		}
	}
	va_end(list);
	return AllRight;
}
