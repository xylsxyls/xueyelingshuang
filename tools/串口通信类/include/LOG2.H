#ifndef __LOG2_H__
#define __LOG2_H__

#include <stdio.h>
#include <string>

using namespace std;


/// LOGTYPE_NULL    不记录日志
/// LOGTYPE_DEBUG   记录DEBUG日志
/// LOGTYPE_ERROR   记录ERROR日志
/// LOGTYPE_SPECIAL 记录特殊日志（硬件等）
typedef enum{LOGTYPE_NULL=0,LOGTYPE_DEBUG=1<<0,LOGTYPE_ERROR=1<<1,LOGTYPE_SPECIAL=1<<2}LOG_TYPE;

#define FILE_IMPORTANT	0x01 /*使用writefile方式记录*/

#define _GLOBAL_FUN_LOG 

#define DASH_LINE   "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
#define DASH_LINE2  "========================================================================"
#define ENTER_LINE  "\n"

#define SET_LOGTYPE(type) SetLogType(type)

#define LOG													WriteLog2

#define LOG_DEBUG2(filename,title,content)					WriteLog(LOGTYPE_DEBUG,title,content,filename)
#define LOG_ERROR(filename,title,content)					WriteLog(LOGTYPE_ERROR,title,content,filename)
#define LOG_SPECIAL(filename,title,content)					WriteLog(LOGTYPE_SPECIAL,title,content,filename)

#define LOG_DEBUG_BY_PATH(title,content,filename,filepath)	WriteLog(LOGTYPE_DEBUG,title,content,filename,filepath)
#define LOG_ERROR_BY_PATH(title,content,filename,filepath)	WriteLog(LOGTYPE_ERROR,title,content,filename,filepath)
#define LOG_SPECIAL_BY_PATH(title,content,filename,filepath)	WriteLog(LOGTYPE_SPECIAL,title,content,filename,filepath)


CString _GLOBAL_FUN_LOG GetPathLogFile(CString strNameLogFile,CString strPathLogFile); //获取日志文件（全局路径）;默认路径为*.EXE下面EXE/LOG/日期/
BOOL _GLOBAL_FUN_LOG WriteFileEx(UINT uType, string strFile, string strAddLog, char* pData, int iLen=-1, BOOL bHex=FALSE);
void _GLOBAL_FUN_LOG WriteLog(LOG_TYPE type,CString strTitle,CString strContent,CString strNameLogFile,CString strPathLogFile=""); //记录日志;默认路径为*.EXE下面EXE/LOG/日期/
void _GLOBAL_FUN_LOG SetLogType(LOG_TYPE eLogType=LOGTYPE_NULL);
void _GLOBAL_FUN_LOG WriteLog2(LOG_TYPE type,CString strName,CString strTitle,LPCTSTR lpszFormat,...);

















































#endif