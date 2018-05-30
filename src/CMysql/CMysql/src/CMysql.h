#pragma once
#include "CMysqlMacro.h"
#include <string>
#include <stdint.h>

class CMysqlManager;
class CHostIP;
class CUser;
class CDataBase;
class CTable;
//在删除表之后必须紧接着转到另一张表再进行操作，否则会报错
//bool ifExists = 1，表示默认必定创建或销毁，如果手动赋值0，则带有是否存在的判断，并根据情况返回相应错误值
//CIP，CUser，CDataBase都具有独立能力，可以开用户，开数据库，开表，每个都会做一个连接，例如CIP做的连接专门用于开用户
//int返回值代表的是数据库里的返回值，如果错误返回错误值
class CMysqlAPI CMysql
{
public:
	CMysql(const std::string& IP = "127.0.0.1", int32_t port = 3306, const std::string& User = "root", const std::string& PassWord = "", const std::string& dbName = "test");
	~CMysql();

public:
	//开IP的接口，并初始化CIP，用于管理User
	CHostIP* OpenHostIPInterface();
	CHostIP* OpenHostIPInterface(const std::string& IP, int32_t port = 3306);
	//开User的接口，用于管理数据库
	CUser* OpenUserInterface();
	CUser* OpenUserInterface(const std::string& User, const std::string& PassWord);
	//开数据库接口，用于管理表
	CDataBase* OpenDataBaseInterface();
	CDataBase* OpenDataBaseInterface(const std::string& dbName, bool AutoCommit = 1);
	//打开表接口，打开的时候会把该表的所有信息全部放到内存中，如果不想全部取出，可以使用数据库接口
	CTable* OpenTableInterface(const std::string& TableName, bool AutoCommit = 1);

public:
	//转到另一个IP的数据库
	//void SelectOtherIPDataBase(std::string IP,std::string User,std::string PassWord,std::string dbName,int32_t port = 3306);

	//选择表进行操作
	//void SelectTable(std::string TableName);

public:
	//这里管理者用的是同一个，但是数据库是一个类一条线，所以需要分开定义
	//CMysql的管理者，用来内存管理和函数管理
	CMysqlManager *m_pMysqlManager;
	//记录现在的port
	int32_t m_port;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//记录现在所在的IP
	std::string m_ip;
	//记录现在的User
	std::string m_user;
	//记录现在的密码
	std::string m_passWord;
	//记录现在的数据库名
	std::string m_dbName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

