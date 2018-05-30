#include <SDKDDKVer.h>
#include "CMysqlManager.h"
#include "CHostIP.h"
#include "CUser.h"
#include "CDataBase.h"
#include "CTable.h"
#include "mysql.h"

CMysqlManager::CMysqlManager()
{

}

CMysqlManager::~CMysqlManager()
{
	//释放listIP
	for (auto itIP = m_listIP.begin(); itIP != m_listIP.end(); itIP++)
	{
		delete *itIP;
	}
	m_listIP.clear();

	//释放listUser
	for (auto itUser = m_listUser.begin(); itUser != m_listUser.end(); itUser++)
	{
		delete *itUser;
	}
	m_listUser.clear();

	//释放listDataBase
	for (auto itDataBase = m_listDataBase.begin(); itDataBase != m_listDataBase.end(); itDataBase++)
	{
		delete *itDataBase;
	}
	m_listDataBase.clear();

	//释放listTable
	for (auto itTable = m_listTable.begin(); itTable != m_listTable.end(); itTable++)
	{
		delete *itTable;
	}
	m_listTable.clear();
}

bool CMysqlManager::DeleteOne(CHostIP* pDelete)
{
	//关闭类内连接线
	if (pDelete->m_mysql != 0)
	{
		mysql_close(pDelete->m_mysql);
	}
	for (auto itIP = m_listIP.begin(); itIP != m_listIP.end(); itIP++)
	{
		if (*itIP == pDelete)
		{
			//释放指针
			delete pDelete;
			//释放list里的指针
			m_listIP.erase(itIP);
			return 1;
		}
	}
	printf("无指定指针，CIP*");
	return 0;
}

bool CMysqlManager::DeleteOne(CUser* pDelete)
{
	//关闭类内连接线
	if (pDelete->m_mysql != 0)
	{
		mysql_close(pDelete->m_mysql);
	}
	for (auto itUser = m_listUser.begin(); itUser != m_listUser.end(); itUser++)
	{
		if (*itUser == pDelete)
		{
			//释放指针
			delete pDelete;
			//释放list里的指针
			m_listUser.erase(itUser);
			return 1;
		}
	}
	printf("无指定指针，CUser*");
	return 0;
}

bool CMysqlManager::DeleteOne(CDataBase* pDelete)
{
	//关闭类内连接线
	if (pDelete->m_mysql != 0)
	{
		mysql_close(pDelete->m_mysql);
	}
	for (auto itDataBase = m_listDataBase.begin(); itDataBase != m_listDataBase.end(); itDataBase++)
	{
		if (*itDataBase == pDelete)
		{
			//释放指针
			delete pDelete;
			//释放list里的指针
			m_listDataBase.erase(itDataBase);
			return 1;
		}
	}
	printf("无指定指针，CDataBase*");
	return 0;
}

bool CMysqlManager::DeleteOne(CTable* pDelete)
{
	//关闭类内连接线
	if (pDelete->m_pDataBase->m_mysql != 0)
	{
		mysql_close(pDelete->m_pDataBase->m_mysql);
	}
	for (auto itTable = m_listTable.begin(); itTable != m_listTable.end(); itTable++)
	{
		if (*itTable == pDelete)
		{
			//释放指针
			delete pDelete;
			//释放list里的指针
			m_listTable.erase(itTable);
			return 1;
		}
	}
	printf("无指定指针，CTable*");
	return 0;
}