#pragma once
#include <SDKDDKVer.h>
#include <afx.h>
#include "mysql.h"
#include "CMysqlManager.h"
#include "CTable.h"
#include "vec.inl"

class CTable;
class CMysqlManager;


class __declspec(dllexport) CDataBase{
	//数据库管表
public:
	MYSQL* mysql;
	CMysqlManager* pMysqlManager;

public:
	
	CString Ip;
	int port;
	CString User;
	CString PassWord;
	CString dbName;

public:
	CDataBase(CMysqlManager* pMysqlManager,CString Ip,int port,CString User,CString PassWord,CString dbName);
	CDataBase(CMysqlManager* pMysqlManager,CString dbName);
	~CDataBase();

public:
	//建表
	CTable* CreateTable(CString TableName,BOOL ifExists = 1);
	//删表
	void DropTable(CString TableName,BOOL ifExists = 1);
	//打开表接口
	CTable* OpenTableInterface(CString TableName);
	//关闭表接口
	void CloseTableInterface(CTable* pTable);
};