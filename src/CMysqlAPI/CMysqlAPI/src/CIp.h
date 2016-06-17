#pragma once
#include <SDKDDKVer.h>
#include <afx.h>
#include "mysql.h"
#include "CMysqlManager.h"
#include "CUser.h"
#include "CDataBase.h"
#include "vec.inl"

class CUser;
class CDataBase;
class CMysqlManager;

class __declspec(dllexport) CIp{
	//Ip管用户
	//在CIp被创建的时候管理者Ip和port就必须被初始化，内部的连接线也必须被初始化
public:
	//CIp中的数据库连接线
	MYSQL *mysql;
	//使用CMysql中的管理者进行管理
	CMysqlManager *pMysqlManager;
public:
	CString Ip;
	int port;

public:
	CIp(CMysqlManager *pMysqlManager,CString Ip,int port);
	~CIp();

public:
	//转到另一个用户的数据库
	CDataBase* OpenUserDataBaseInterface(CString User,CString PassWord,CString dbName);
	void CloseDataBaseInterface(CDataBase* pCDataBase);
};