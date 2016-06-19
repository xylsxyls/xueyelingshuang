#pragma once
#include <SDKDDKVer.h>
#include <afx.h>
#include "mysql.h"
#include "CIp.h"
#include "CUser.h"
#include "CDataBase.h"
#include "CTable.h"
#include "CMysqlManager.h"
#include "API.h"
#include <vector>
using namespace std;

typedef int MultiBool;

class CMysqlManager;
class CIp;
class CUser;
class CDataBase;
class CTable;

class API CMysql{
	//所有BOOL为返回值的函数，1表示成功，0表示失败
	//MysqlSucceed为关键判断值，一旦出现错误则变为0，之后所有成员语句不执行
	//在删除表之后必须紧接着转到另一张表再进行操作，否则会报错
	//BOOL ifExists = 1，表示默认必定创建或销毁，如果手动赋值0，则带有是否存在的判断，并根据情况返回相应错误值
	//CIp，CUser，CDataBase都具有独立能力，可以开用户，开数据库，开表，每个都会做一个连接，例如CIp做的连接专门用于开用户

public:
	//这里管理者用的是同一个，但是数据库是一个类一条线，所以需要分开定义

	//主要操作数据库的指针
	MYSQL *mysql;
	//CMysql的管理者，用来内存管理和函数管理
	CMysqlManager *pMysqlManager;
	//记录现在所在的Ip
	CString Ip;
	//记录现在的port
	int port;
	//记录现在的User
	CString User;
	//记录现在的密码
	CString PassWord;
	//记录现在的数据库名
	CString dbName;

public:
	CMysql(CString IP = "127.0.0.1",CString User = "root",CString PassWord = "root",CString dbName = "test",int port = 3306);
	~CMysql();

public:
	//开Ip的接口，并初始化CIp，用于管理User
	CIp* OpenIpInterface();
	CIp* OpenIpInterface(CString Ip,int port = 3306);
	//开User的接口，用于管理数据库
	CUser* OpenUserInterface();
	CUser* OpenUserInterface(CString User,CString PassWord);
	//开数据库接口，用于管理表
	CDataBase* OpenDataBaseInterface();
	CDataBase* OpenDataBaseInterface(CString dbName);
	//开表接口，用于管理字段和记录
	CTable* OpenTableInterface(CString TableName);

public:
	
	
	//转到另一个IP的数据库
	//void SelectOtherIPDataBase(CString IP,CString User,CString PassWord,CString dbName,int port = 3306);
	
	//选择表进行操作
	//void SelectTable(CString TableName);
	

};