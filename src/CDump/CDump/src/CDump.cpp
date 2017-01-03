#include <SDKDDKVer.h>
#include "CDump.h"
#include <tchar.h>
#include <Windows.h>
#include <Dbghelp.h>

void CreateDumpFile(LPCSTR lpstrDumpFilePathName, EXCEPTION_POINTERS *pException){  
	//创建Dump文件  
	HANDLE hDumpFile = CreateFile(lpstrDumpFilePathName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);  
	//Dump信息
	MINIDUMP_EXCEPTION_INFORMATION dumpInfo;  
	dumpInfo.ExceptionPointers = pException;  
	dumpInfo.ThreadId = GetCurrentThreadId();  
	dumpInfo.ClientPointers = TRUE;  
	//写入Dump文件内容  
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);  
	CloseHandle(hDumpFile);
	return;
}  

LPTOP_LEVEL_EXCEPTION_FILTER WINAPI MyDummySetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter){
	return NULL;
}

BOOL PreventSetUnhandledExceptionFilter(){
	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
	if (hKernel32 ==   NULL) return FALSE;
	void *pOrgEntry = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
	if(pOrgEntry == NULL) return FALSE;
	unsigned char newJump[100] = {};
	DWORD dwOrgEntryAddr = (DWORD) pOrgEntry;
	dwOrgEntryAddr += 5; //add 5 for 5 op-codes for jmp far
	void *pNewFunc = &MyDummySetUnhandledExceptionFilter;
	DWORD dwNewEntryAddr = (DWORD) pNewFunc;
	DWORD dwRelativeAddr = dwNewEntryAddr -  dwOrgEntryAddr;
	newJump[0] = 0xE9; //JMP absolute
	memcpy(&newJump[1], &dwRelativeAddr, sizeof(pNewFunc));
	SIZE_T bytesWritten;
	BOOL bRet = WriteProcessMemory(GetCurrentProcess(),    pOrgEntry, newJump, sizeof(pNewFunc) + 1, &bytesWritten);
	return bRet;
}

string szMbsFile = "";

LONG WINAPI UnhandledExceptionFilterEx(struct _EXCEPTION_POINTERS *pException){
	CreateDumpFile(szMbsFile.c_str(), pException);
	// TODO: MiniDumpWriteDump
	FatalAppExit(-1, _T("Fatal Error"));
	return EXCEPTION_CONTINUE_SEARCH;
}	

void RunCrashHandler(){
	SetUnhandledExceptionFilter(UnhandledExceptionFilterEx);
	PreventSetUnhandledExceptionFilter();
	return;
}

void CDump::DeclareDumpFile(string DumpFilePath){
	szMbsFile = DumpFilePath;
	RunCrashHandler();
	return;
}