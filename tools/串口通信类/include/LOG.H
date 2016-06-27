#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <string>
#include "ShareMemory.h"

using namespace std;

#define LOG_TYPE_NONE		0x00
#define LOG_TYPE_IMPORTANT	0x01 /*使用writefile方式记录*/
#define LOG_TYPE_ERROR		0x02
#define LOG_TYPE_NORMAL		0x04
#define LOG_TYPE_DETAILED	0x08
#define LOG_TYPE_ALL		0xFF


/* PATH.H 包含了目录相关的操作
	
	//检查路径是否存在,可以选择不存在创建
	inline BOOL CheckPath(LPTSTR pszPath, BOOL bCreate)
	//检查目录是否存在 注意pszBuffer不能以'\\'结尾,否则会失败
	inline BOOL FindDir(LPTSTR pszBuffer)
	//删除指定目录指定天数前的旧日志
	BOOL DeleteLogFiles(LPTSTR pszPath, UINT nDays=30)
	获取当前程序路径
	char* GetExePath()
	获取当前模块所在的盘符
	char* GetModuleDisk()
	获取当前模块所在的路径
	char* GetModulePath()
	获取当前模块名
	char* GetModuleName()
	获取当前模块名key(即模块名去除.后面的内容)
	char* GetModuleNameKey()
 */

BOOL FindDir(LPTSTR pszBuffer);
BOOL FindFile(LPTSTR pszFile);
BOOL CheckPath(LPTSTR pszPath, BOOL bCreate = TRUE);
char* GetExePath();
char* GetModuleDisk();
char* GetModulePath();
char* GetModuleName();
char* GetModuleNameKey();
BOOL DeleteLogFiles(LPTSTR pszPath, UINT nDays=30);
char* GetFileLine(char* szFileName, int iLine);


/*
CLog 类 用于实现日志的简单记录 --add by daguoc
设置日志记录模式
void SetLogMode(UINT nMode, UINT nLevel, UINT nEncrypt);
设置自动生成文件名参数
void SetFileNameKey(string strNameKey, string strPath="", string strSuffix="", UINT nAddDate=1);
记录日志
int Log(LPCTSTR lpszFile, LPCTSTR lpszFormat, ...);
记录16进制日志
int LogHex(LPCTSTR lpszFile, LPCTSTR lpszHead, unsigned char* lpszData, UINT nLen);

  使用步骤：
  应用1：
  #include "LOG.H"
  LOG_SET_MODE(2); // 设置模式 0 正常记录 1添加日期记录 2添加时间记录 默认为2
  LOG_SET_NAMEKEY("testlog","d:\\", "txt", 1); // 设置文件名 
  参数1 文件名关键字
  参数2 文件存放路径
  参数3 文件后缀
  参数4 文件名是否带日期(YYYYMMDD)
  LOG(LOG_TYPE_NORMAL, "%s%s%s%d", "a", "b", "c", 1);		 // 记录日志
  参数1 日志文件(使用相对或绝对路径) 为空时使用LOG_SET_NAMEKEY设置的文件名
  参数2 格式化字符串
  ...
  参数n 附带参数
  LOGHEX(LOG_TYPE_NORMAL, ">", p, 10);						 // 记录日志 使用16进制格式记录
  参数1 日志文件(使用相对或绝对路径) 为空时使用LOG_SET_NAMEKEY设置的文件名
  参数2 日志头部添加内容
  参数3 数据位置 unsigned char* 
  参数4 记录数据长度
  通过使用LOG方法实现日志的记录
  应用2：
  对于项目中需要写多个不同日志，可以自己定义CLog对象使用
*/

#define MAX_BUFF 1024


class CLog
{
public:
	CLog();
	CLog(string strPath, string strKey, string strSuffix="", UINT uLogType=LOG_TYPE_ALL, 
		BOOL bDelPathFile=FALSE, UINT uDelDays=30, BOOL bCreatePath=TRUE);
	~CLog();

	
private:
	/* 定义记录日志追加时间信息 取累加值
	0 正常模式 
	1 记录日期(YYYY-MM-DD)
	2 记录时间(HH:Mi:SS.xxx)
	*/ 
	UINT m_nLogMode;
	
	/* 记录日志类型
	0x00 不记录日志
	0x01 直接写文件(数据重要)
	0x02 错误日志
	0x04 一般日志
	0x08 详细日志
	0xFF 记录所有日志
	*/
	UINT m_nLogType;

	/* 是否加密
	1 加密 采用按位取反
	0 不加密
	*/
	UINT m_nEncrypt;
	
	// 下面用于自动生成文件名 生成规则：m_strFilePath + m_strFileNameKey + 当前日期(YYYYMMDD) + "." + m_strSuffix
	string m_strFilePath;//文件存放路径
	string m_strFileNameKey;//文件关键词 
	UINT m_nFileNameAddDate;//文件名追加日期 1为需要 0不需要
	string m_strSuffix;//文件后缀(扩展名)

	ThreadMutex m_Mutex;

public:
	void SetLogMode(UINT nMode);
	void SetLogType(UINT nType);
	void SetFileNameKey(string strNameKey, string strPath="", string strSuffix="", UINT nFileNameAddDate=1);
	int Log(UINT nLogType, LPCTSTR lpszFormat, ...);
	int LogHex(UINT nLogType, string strHead, char* lpszData, UINT nLen);

	BOOL WriteLogFile(UINT uType, string strFile, string strAddLog, char* pData, int iLen=-1, BOOL bHex=FALSE);

private:
	string GetFileFullPathName(string strAdd="");
	string GetAddLog(UINT uType=0);
};

// 定义一个实体，用于下面宏定义使用
extern CLog my_log;

#define LOG_SET_MODE(a) my_log.SetLogMode(a)
#define LOG_SET_NAMEKEY my_log.SetFileNameKey
#define LOG_SET_TYPE my_log.SetLogType
#define LOG my_log.Log
#define LOGHEX my_log.LogHex
#define FILELINE GetFileLine(__FILE__, __LINE__)


#define REPORT_ERROR_PARAM_2(e, a)	{my_log.Log(LOG_TYPE_ERROR, "%s%s%s", FILELINE, (e), (a));}
#define REPORT_ERROR_PARAM_1(e)	{my_log.Log(LOG_TYPE_ERROR, "%s%s", FILELINE, (e));}
#define OUT_DEBUG_PARAM_1(e)	{my_log.Log(LOG_TYPE_NORMAL, "%s%s", FILELINE, (e));}
#define OUT_DEBUG_PARAM_0()	{my_log.Log(LOG_TYPE_NORMAL, "%s", FILELINE);}
#define WRITE_LOG_FILE(f, d) my_log.WriteLogFile(0, (f), "", (d))

#endif //__LOG_H__