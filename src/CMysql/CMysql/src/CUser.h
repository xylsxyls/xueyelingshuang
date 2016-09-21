#pragma once
#include <afxwin.h>
#include "mysql.h"

class CMysqlManager;
class CDataBase;
class CUser{
	//用户管数据库
	//CUser被创建时必须初始化所有成员变量
public:
	MYSQL *mysql;
	CMysqlManager* pMysqlManager;

public:
	CString IP;
	int port;
	CString User;
	CString PassWord;

public:
	CUser(CMysqlManager *pMysqlManager,CString IP,int port,CString User,CString PassWord);
	void Close();

public:

	//创建数据库，返回1表示成功，ifExists = 1表示无论存不存在都创建，ifExists = 0表示，如果不存在则创建，存在则返回错误
	//创建之后自动转到这个数据库
	CDataBase* CreateDataBase(CString NewdbName,bool ifExists = 1);
	//销毁数据库，同时释放pDataBase空间
	int DropDataBase(CDataBase *pDataBase);
	//打开需要操作的数据库
	CDataBase* OpenDataBaseInterface(CString dbName);
	//关闭数据库
	void CloseDataBaseInterface(CDataBase *pDataBase);
};