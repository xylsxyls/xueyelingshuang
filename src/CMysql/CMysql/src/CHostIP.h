#pragma once
#include <afxwin.h>
#include "mysql.h"
#include "CMysqlMacro.h"

class CDataBase;
class CMysqlManager;
class CMysqlAPI CHostIP{
	//IP管用户
	//在CIP被创建的时候管理者IP和port就必须被初始化，内部的连接线也必须被初始化
public:
	//CIP中的数据库连接线
	MYSQL *mysql;
	//使用CMysql中的管理者进行管理
	CMysqlManager *pMysqlManager;
public:
	CString IP;
	int port;

public:
	CHostIP(CMysqlManager *pMysqlManager,CString IP,int port);

public:
	//转到另一个用户的数据库
	CDataBase* OpenUserDataBaseInterface(CString User,CString PassWord,CString dbName);
	void CloseDataBaseInterface(CDataBase* pCDataBase);
};