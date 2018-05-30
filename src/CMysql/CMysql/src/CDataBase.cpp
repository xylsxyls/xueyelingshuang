#include <SDKDDKVer.h>
#include "CDataBase.h"
#include "CMysqlManager.h"
#include "CTable.h"
#include "mysql.h"
#include "CStringManager/CStringManagerAPI.h"

CDataBase::CDataBase(bool* bSucceed, CMysqlManager* pMysqlManager, const std::string& ip, int32_t port, const std::string& user, const std::string& password, const std::string& dbName)
{
	//���ⲿ���������ָ��
	m_pMysqlManager = pMysqlManager;
	//�Ȱ��¿���ָ��ŵ�������
	m_pMysqlManager->m_listDataBase.push_back(this);

	m_ip = ip;
	m_port = port;
	m_user = user;
	m_password = password;
	m_dbName = dbName;
	m_mysql = mysql_init(NULL);

	MYSQL *isSucceed = mysql_real_connect(m_mysql, ip.c_str(), user.c_str(), password.c_str(), dbName.c_str(), port, NULL, 0);
	//�������ʧ�����ͷţ����ǲ��ͷŹ����ߣ�������ֻ����CMysql���ͷ�ʱ�Ż��ͷ�
	if (isSucceed == NULL)
	{
		m_pMysqlManager->DeleteOne(this);
		*bSucceed = 0;
	}
	*bSucceed = 1;
}

CTable* CDataBase::CreateTable(const std::string& TableName, const std::string& FieldName, int32_t type, int32_t length, bool ifExists, bool AutoCommit)
{
	int32_t nIsSucceed = -1;
	std::string strType;
	if (type == 1) strType = "varchar";
	else if (type == 2) strType = "int";
	else if (type == 3) strType = "double";
	std::string strLength;
	strLength = CStringManager::Format("%d", length);
	//����Ĭ����һ��test��int�ֶΣ���Ϊ���ݿ��޷������ձ�
	if (ifExists == 0) nIsSucceed = mysql_query(m_mysql, ("create table " + TableName + "(" + FieldName + " " + strType + "(" + strLength + "))").c_str());
	if (ifExists == 1) nIsSucceed = mysql_query(m_mysql, ("create table if not exists " + TableName + "(" + FieldName + " " + strType + "(" + strLength + "))").c_str());

	CTable* pTable = NULL;
	if (nIsSucceed == 0)
	{
		//�¿�һ����ӿڲ�Ϊ�����ڲ�����һ��������
		pTable = new CTable(m_pMysqlManager, this, TableName, 1);
		//�����ݿ�����ڲ�����������������һ���Ž�CTable��
		mysql_real_connect(pTable->m_pDataBase->m_mysql, m_ip.c_str(), m_user.c_str(), m_password.c_str(), m_dbName.c_str(), m_port, NULL, 0);
	}
	return pTable;
}

int32_t CDataBase::DropTable(const std::string& TableName, bool ifExists)
{
	int32_t nIsSucceed = -1;
	//ɾ��
	if (ifExists == 0) nIsSucceed = mysql_query(m_mysql, ("drop table " + TableName).c_str());
	else if (ifExists == 1) nIsSucceed = mysql_query(m_mysql, ("drop table if exists " + TableName).c_str());

	return nIsSucceed;
}

CTable* CDataBase::OpenTableInterface(const std::string& TableName, bool AutoCommit)
{
	CTable* pTable = NULL;
	//����0��ʾȷʵ�����ű�
	mysql_query(m_mysql, ("SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA='" + m_dbName + "' AND TABLE_NAME='" + TableName + "'").c_str());
	//������������ == 1˵��ȷʵ�����ű�
	if (mysql_store_result(m_mysql)->row_count == 1)
	{
		//�¿�һ����ӿڲ�Ϊ�����ڲ�����һ��������
		pTable = new CTable(m_pMysqlManager, this, TableName);
		//�����ݿ�����ڲ�����������������һ���Ž�CTable��
		mysql_real_connect(pTable->m_pDataBase->m_mysql, m_ip.c_str(), m_user.c_str(), m_password.c_str(), m_dbName.c_str(), m_port, NULL, 0);
	}
	pTable->Refresh();
	return pTable;
}

void CDataBase::CloseTableInterface(CTable* pTable)
{
	m_pMysqlManager->DeleteOne(pTable);
	return;
}