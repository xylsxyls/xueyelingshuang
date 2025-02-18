#include <SDKDDKVer.h>
#include "CHostIP.h"
#include "CDataBase.h"
#include "CMysqlManager.h"
#include "mysql.h"

CHostIP::CHostIP(CMysqlManager* pMysqlManager, const std::string& ip, int32_t port)
{
	//从外部导入管理者指针
	m_pMysqlManager = pMysqlManager;
	//先把新开的指针放到容器里
	m_pMysqlManager->m_listIP.push_back(this);

	m_ip = ip;
	m_port = port;
	m_mysql = mysql_init(NULL);
}

CDataBase* CHostIP::OpenUserDataBaseInterface(const std::string& user, const std::string& password, const std::string& dbName)
{
	bool bSucceed = 0;
	CDataBase* pDataBase = new CDataBase(&bSucceed, m_pMysqlManager, m_ip, m_port, user, password, dbName);
	if (bSucceed == 0)
	{
		pDataBase = 0;
	}
	return pDataBase;
}

void CHostIP::CloseDataBaseInterface(CDataBase* pDataBase)
{
	m_pMysqlManager->DeleteOne(pDataBase);
	return;
}