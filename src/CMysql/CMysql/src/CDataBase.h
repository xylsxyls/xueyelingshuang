#pragma once
#include <afxwin.h>
#include "mysql.h"
#include "CMysqlManager.h"
#include "CTable.h"

class CTable;
class CMysqlManager;


class CDataBase{
	//数据库管表
public:
	MYSQL* mysql;
	CMysqlManager* pMysqlManager;

public:
	CString IP;
	int port;
	CString User;
	CString PassWord;
	CString dbName;

public:
	//如果数据库连接失败则会自动释放，包括管理者里的指针释放，用bSucceed标识，但是new出来的依然有值，需要在外部置0
	CDataBase(bool* bSucceed,CMysqlManager* pMysqlManager,CString IP,int port,CString User,CString PassWord,CString dbName);

public:
	//建表
	CTable* CreateTable(CString TableName,CString FieldName,BOOL type,int length,bool ifExists = 1,bool AutoCommit = 1);
	//删表
	int DropTable(CString TableName,bool ifExists = 1);
	//打开表接口
	CTable* OpenTableInterface(CString TableName,bool AutoCommit = 1);
	//关闭表接口
	void CloseTableInterface(CTable* pTable);
};