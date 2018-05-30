#pragma once
#include "CMysqlMacro.h"
#include <string>
#include <stdint.h>

class CDataBase;
class CMysqlManager;
struct st_mysql;
typedef st_mysql MYSQL;
//ip管用户
//在CIP被创建的时候管理者ip和port就必须被初始化，内部的连接线也必须被初始化
class CMysqlAPI CHostIP
{
public:
	CHostIP(CMysqlManager *pMysqlManager, const std::string& ip, int32_t port);

public:
	//转到另一个用户的数据库
	CDataBase* OpenUserDataBaseInterface(const std::string& user, const std::string& password, const std::string& dbName);
	void CloseDataBaseInterface(CDataBase* pCDataBase);

public:
	//CIP中的数据库连接线
	MYSQL* m_mysql;
	//使用CMysql中的管理者进行管理
	CMysqlManager* m_pMysqlManager;
	int32_t m_port;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_ip;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};