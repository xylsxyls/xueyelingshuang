#pragma once
#include <afxwin.h>

class CMysqlManager;
class CIP;
class CUser;
class CDataBase;
class CTable;
class CMysql{
	//在删除表之后必须紧接着转到另一张表再进行操作，否则会报错
	//bool ifExists = 1，表示默认必定创建或销毁，如果手动赋值0，则带有是否存在的判断，并根据情况返回相应错误值
	//CIP，CUser，CDataBase都具有独立能力，可以开用户，开数据库，开表，每个都会做一个连接，例如CIP做的连接专门用于开用户
	//int返回值代表的是数据库里的返回值，如果错误返回错误值
public:
	//这里管理者用的是同一个，但是数据库是一个类一条线，所以需要分开定义
	//CMysql的管理者，用来内存管理和函数管理
	CMysqlManager *pMysqlManager;
	//记录现在所在的IP
	CString IP;
	//记录现在的port
	int port;
	//记录现在的User
	CString User;
	//记录现在的密码
	CString PassWord;
	//记录现在的数据库名
	CString dbName;

public:
	CMysql(CString IP = "127.0.0.1",int port = 3306,CString User = "root",CString PassWord = "root",CString dbName = "test");
	~CMysql();

public:
	//开IP的接口，并初始化CIP，用于管理User
	CIP* OpenIPInterface();
	CIP* OpenIPInterface(CString IP,int port = 3306);
	//开User的接口，用于管理数据库
	CUser* OpenUserInterface();
	CUser* OpenUserInterface(CString User,CString PassWord);
	//开数据库接口，用于管理表
	CDataBase* OpenDataBaseInterface();
	CDataBase* OpenDataBaseInterface(CString dbName,bool AutoCommit = 1);
	//打开表接口，打开的时候会把该表的所有信息全部放到内存中，如果不想全部取出，可以使用数据库接口
	CTable* OpenTableInterface(CString TableName,bool AutoCommit = 1);

public:
	//转到另一个IP的数据库
	//void SelectOtherIPDataBase(CString IP,CString User,CString PassWord,CString dbName,int port = 3306);
	
	//选择表进行操作
	//void SelectTable(CString TableName);
};

