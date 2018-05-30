#pragma once
#include "CMysqlMacro.h"
#include <string>
#include <stdint.h>

class CMysqlManager;
class CDataBase;
struct st_mysql;
typedef st_mysql MYSQL;
//用户管数据库
//CUser被创建时必须初始化所有成员变量
class CMysqlAPI CUser
{
public:
	CUser(CMysqlManager *pMysqlManager, const std::string& IP, int32_t port, const std::string& User, const std::string& PassWord);
	void Close();

public:

	//创建数据库，返回1表示成功，ifExists = 1表示无论存不存在都创建，ifExists = 0表示，如果不存在则创建，存在则返回错误
	//创建之后自动转到这个数据库
	CDataBase* CreateDataBase(const std::string& NewdbName, bool ifExists = 1);
	//销毁数据库，同时释放pDataBase空间
	int32_t DropDataBase(CDataBase *pDataBase);
	//打开需要操作的数据库
	CDataBase* OpenDataBaseInterface(const std::string& dbName);
	//关闭数据库
	void CloseDataBaseInterface(CDataBase *pDataBase);

public:
	MYSQL *m_mysql;
	CMysqlManager* m_pMysqlManager;
	int32_t m_port;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_ip;
	std::string m_user;
	std::string m_passWord;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};