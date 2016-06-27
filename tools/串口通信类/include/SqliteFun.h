#if !defined(AFX_DBSQLITEFUN_H__DC7062D9_6FF6_4E17_BBD9_87ECF348542B__INCLUDED_)
#define AFX_DBSQLITEFUN_H__DC7062D9_6FF6_4E17_BBD9_87ECF348542B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DBSqliteFun.h : header file
//

#include <sqlite3.h>
#include "CommTbl.h"

/////////////////////////////////////////////////////////////////////////////
typedef int (*Sqlite3_Callback)(void*,int,char**, char**);

class CSqliteFun
{
// Construction
public:
	CSqliteFun();
	virtual ~CSqliteFun();
private:
	sqlite3* m_pSqliteDB;
	std::string	 m_strError;	//专指执行sql语句出错描述
	
public:
	int OpenDatabase(CString strDbPath);	///返回为0 表示打开成功
	int CloseDatabase();					///返回0 表示关闭成功
	int ExeSqlTransaction(std::string strSql);		///执行sql语句
	int ExeSqlTransaction(CCommTbl<std::string> vctSql, bool bRoll=true);		///执行sql语句
	int CheckTable(const char* pTableName);		// 检测表是否存在 1存在 0不存在
	CCommTbl<CCommTbl<std::string>> GetTable(const char *pSql);// 获取SQL语句结果数据
	std::string GetLastError() {return m_strError;}

private:
	const char* Sqlite3_errmsg() {return sqlite3_errmsg(m_pSqliteDB);}
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DBSQLITEFUN_H__DC7062D9_6FF6_4E17_BBD9_87ECF348542B__INCLUDED_)
