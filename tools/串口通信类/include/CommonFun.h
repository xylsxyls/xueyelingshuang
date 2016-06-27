#ifndef _COMMON_FUN_H_  
#define _COMMON_FUN_H_ 

#include "stdafx.h"
#include <string> 
#include <vector>
#include <afxinet.h>

using namespace std;

/************************************************************************
* WM_USER: 0x0400-0x7FFF 
************************************************************************/

typedef enum{DATE_LONG,DATE_LONG2 ,DAY_NORMAL, TIME_NORMAL, DATE_NORMAL, DAY_SHORT, DAY_LONG, TIME_LONG, TIME_SHORT1,TIME_SHORT2,MONTH_NORMAL, MONTH_LONG, BANK_NORMAL}TIME_TYPE;

std::vector<CString> SplitString(CString strInfo, CString strFlag);
std::vector<string> Splitstring(string strInfo, string strFlag);

short StrCopy(char* szDest, const char* szSrc, int nDestMaxLen);
short MemCopy(void* pDest, const void* pSrc, int nSrcLen, int nDestMaxLen);

// 转换成BCD字串
int ConvertBCD(BYTE* pSrc, int nLen, BYTE* pDest);
// BCD字串转换为二进制
int ConvertBIN(BYTE* pSrc, BYTE* pDest);

//获取当前时间串
string GetCurTime(TIME_TYPE nType, long lSpan=0);
//返回当前时间和 1970-1-1 00:00:00 之间的秒数
unsigned long GetCurTime();
/************************************************************************\
* 比较时间差                                                             *
* 入参：%d-%d-%d %d:%d:%d                                                *
* 返回值：>0表示现在时间大于比较时间N天/秒即tCompare为N天/秒前日期       *
\************************************************************************/
int CompareDay(CTime tCompare);
int CompareSecond(CTime tCompare);
int CompareDay(CString strCompare);
int CompareSecond(CString strCompare);

//重启系统
BOOL Reboot();
//关闭系统
BOOL ShutDown();
//设置系统当前时间(格式必须为：2012-12-01 12:00:00)
BOOL SetSysTime(char* pCurTime);

//关闭所有指定进程
BOOL KillProcess(CString  strKillProcessName);
//关闭指定进程
BOOL KillProcess(DWORD  dwProcessID);
// 返回第一个进程ID 未找到返回0 失败-1
long FindProcess(CString  strProcessName);

// 启动进程 模态窗口无法隐藏
BOOL StartProcess(CString strFile, CString strParam, UINT uShow, BOOL bBlock);
BOOL StartProcess(CString strFile, CString strParam, UINT uShow, BOOL bBlock, DWORD* dwExitCode);
// 启动进程（直到后台运行 确保服务启动 主要用来启动服务类程序）
BOOL StartProcess(CString strFile, BOOL bHide);
BOOL StartProcess2(CString strFile, BOOL bHide);

// 读注册表信息
CString ReadREG(HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpName);
// 写注册表
BOOL WriteREG(HKEY hKey, LPCTSTR lpSubKey, LPCTSTR lpName, LPCTSTR lpValue);

// 返回磁盘剩余空间（单位字节） -1 失败
ULONGLONG GetDiskSpace(char* pDir);

// 获取指定目录大小
unsigned long GetDirSize(char* pDir);
// 获取CPUID
CString GetCPUID();
// 获取MAC地址（取所有物理和虚拟（不包括无线）MAC）
CString GetMacSum();
// 取 GetCPUID() 和 GetMacSum() 异或的结果
CString GetHsKey();

void ConvertUtf8ToGBK(CString& strUtf8);
void ConvertGBKToUtf8(CString& strGBK);

BOOL CreateAllDirectory(LPCTSTR lpszPath);
CString GetFullPath(CString sFileName);
std::vector<CString> FindFileFullPath(CString strFoldername);
std::vector<CString> FindFileName(CString strFoldername);


char* GetExePath();
HMODULE WINAPI ModuleFromAddress(PVOID pv);
HMODULE GetCurModule();
int GetCurModulePath();
char* GetModuleDisk();
char* GetModulePath();
char* GetModuleName();
char* GetModuleNameKey();
BOOL CheckFileExist(CString strFilePath);

CString GetLastSysErr();
CString GetLocalIpaddress();

BOOL DeleteLog(LPTSTR strPath,UINT uDays);
void DeleteDirectoryForce(CString directory_path);

BYTE CharXORChar(LPBYTE pdata,int nLen);
BYTE Plus(LPBYTE pdata,int nLen,BYTE bChk);

/// 将GBK汉字转化成多字节（GBK编码“欢”“BB B6”转化成“0B 0B 0B 06”）
void GbkTODoubleGbk(char* pIn,LPBYTE pOut,UINT uIn,UINT& uOut);
/// 将GBK汉字转化成多字节（GBK编码“欢1”“BB B6 31”转化成“BB B6 00 31”）
void GbkTOSplitGbk(char* pIn,LPBYTE pOut,UINT uIn,UINT& uOut);

BOOL InputEnglish();
BOOL InputChinese();

void CRC(LPBYTE ptrData,UINT nlen,LPBYTE ptrCRC);

int HTTP_Download(LPCTSTR pFileURLInServer, //待下载文件的URL
			 LPCTSTR pFileLocalFullPath);
int HTTP_UploadFile(LPCTSTR pURL, //负责接收上传操作的页面的URL
			   LPCTSTR pLocalFileName);

CString GetUUID();
CTimeSpan CompareTime(CString strCompare1,CString strCompare2);

#endif;