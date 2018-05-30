#pragma once
#include <string>
#include "CMysqlMacro.h"
#include <stdint.h>

struct st_mysql;
typedef st_mysql MYSQL;
class CTable;
class CMysqlManager;
//数据库管表
class CMysqlAPI CDataBase
{
public:
	//如果数据库连接失败则会自动释放，包括管理者里的指针释放，用bSucceed标识，但是new出来的依然有值，需要在外部置0
	CDataBase(bool* bSucceed, CMysqlManager* pMysqlManager, const std::string& ip, int32_t port, const std::string& user, const std::string& password, const std::string& dbName);

public:
	//建表
	CTable* CreateTable(const std::string& TableName, const std::string& FieldName, int32_t type, int32_t length, bool ifExists = 1, bool AutoCommit = 1);
	//删表
	int32_t DropTable(const std::string& TableName, bool ifExists = 1);
	//打开表接口
	CTable* OpenTableInterface(const std::string& TableName, bool AutoCommit = 1);
	//关闭表接口
	void CloseTableInterface(CTable* pTable);

public:
	MYSQL* m_mysql;
	CMysqlManager* m_pMysqlManager;
	int32_t m_port;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_ip;
	std::string m_user;
	std::string m_password;
	std::string m_dbName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};