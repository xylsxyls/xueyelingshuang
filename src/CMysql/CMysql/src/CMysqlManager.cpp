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
	//�ͷ�listIP
	for (auto itIP = m_listIP.begin(); itIP != m_listIP.end(); itIP++)
	{
		delete *itIP;
	}
	m_listIP.clear();

	//�ͷ�listUser
	for (auto itUser = m_listUser.begin(); itUser != m_listUser.end(); itUser++)
	{
		delete *itUser;
	}
	m_listUser.clear();

	//�ͷ�listDataBase
	for (auto itDataBase = m_listDataBase.begin(); itDataBase != m_listDataBase.end(); itDataBase++)
	{
		delete *itDataBase;
	}
	m_listDataBase.clear();

	//�ͷ�listTable
	for (auto itTable = m_listTable.begin(); itTable != m_listTable.end(); itTable++)
	{
		delete *itTable;
	}
	m_listTable.clear();
}

bool CMysqlManager::DeleteOne(CHostIP* pDelete)
{
	//�ر�����������
	if (pDelete->m_mysql != 0)
	{
		mysql_close(pDelete->m_mysql);
	}
	for (auto itIP = m_listIP.begin(); itIP != m_listIP.end(); itIP++)
	{
		if (*itIP == pDelete)
		{
			//�ͷ�ָ��
			delete pDelete;
			//�ͷ�list���ָ��
			m_listIP.erase(itIP);
			return 1;
		}
	}
	printf("��ָ��ָ�룬CIP*");
	return 0;
}

bool CMysqlManager::DeleteOne(CUser* pDelete)
{
	//�ر�����������
	if (pDelete->m_mysql != 0)
	{
		mysql_close(pDelete->m_mysql);
	}
	for (auto itUser = m_listUser.begin(); itUser != m_listUser.end(); itUser++)
	{
		if (*itUser == pDelete)
		{
			//�ͷ�ָ��
			delete pDelete;
			//�ͷ�list���ָ��
			m_listUser.erase(itUser);
			return 1;
		}
	}
	printf("��ָ��ָ�룬CUser*");
	return 0;
}

bool CMysqlManager::DeleteOne(CDataBase* pDelete)
{
	//�ر�����������
	if (pDelete->m_mysql != 0)
	{
		mysql_close(pDelete->m_mysql);
	}
	for (auto itDataBase = m_listDataBase.begin(); itDataBase != m_listDataBase.end(); itDataBase++)
	{
		if (*itDataBase == pDelete)
		{
			//�ͷ�ָ��
			delete pDelete;
			//�ͷ�list���ָ��
			m_listDataBase.erase(itDataBase);
			return 1;
		}
	}
	printf("��ָ��ָ�룬CDataBase*");
	return 0;
}

bool CMysqlManager::DeleteOne(CTable* pDelete)
{
	//�ر�����������
	if (pDelete->m_pDataBase->m_mysql != 0)
	{
		mysql_close(pDelete->m_pDataBase->m_mysql);
	}
	for (auto itTable = m_listTable.begin(); itTable != m_listTable.end(); itTable++)
	{
		if (*itTable == pDelete)
		{
			//�ͷ�ָ��
			delete pDelete;
			//�ͷ�list���ָ��
			m_listTable.erase(itTable);
			return 1;
		}
	}
	printf("��ָ��ָ�룬CTable*");
	return 0;
}