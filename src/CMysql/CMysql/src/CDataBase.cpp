#include <SDKDDKVer.h>
#include "CDataBase.h"
#include "CMysqlManager.h"
#include "CTable.h"
#include "mysql.h"
#include "CStringManager/CStringManagerAPI.h"

CDataBase::CDataBase(bool* bSucceed, CMysqlManager* pMysqlManager, const std::string& ip, int32_t port, const std::string& user, const std::string& password, const std::string& dbName)
{
	//从外部导入管理者指针
	m_pMysqlManager = pMysqlManager;
	//先把新开的指针放到容器里
	m_pMysqlManager->m_listDataBase.push_back(this);

	m_ip = ip;
	m_port = port;
	m_user = user;
	m_password = password;
	m_dbName = dbName;
	m_mysql = mysql_init(NULL);

	MYSQL *isSucceed = mysql_real_connect(m_mysql, ip.c_str(), user.c_str(), password.c_str(), dbName.c_str(), port, NULL, 0);
	//如果连接失败则释放，但是不释放管理者，管理者只有在CMysql被释放时才会释放
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
	//建表，默认有一个test的int字段，因为数据库无法建立空表
	if (ifExists == 0) nIsSucceed = mysql_query(m_mysql, ("create table " + TableName + "(" + FieldName + " " + strType + "(" + strLength + "))").c_str());
	if (ifExists == 1) nIsSucceed = mysql_query(m_mysql, ("create table if not exists " + TableName + "(" + FieldName + " " + strType + "(" + strLength + "))").c_str());

	CTable* pTable = NULL;
	if (nIsSucceed == 0)
	{
		//新开一个表接口并为他的内部建立一个连接线
		pTable = new CTable(m_pMysqlManager, this, TableName, 1);
		//把数据库里的内部连接线重新再制造一个放进CTable内
		mysql_real_connect(pTable->m_pDataBase->m_mysql, m_ip.c_str(), m_user.c_str(), m_password.c_str(), m_dbName.c_str(), m_port, NULL, 0);
	}
	return pTable;
}

int32_t CDataBase::DropTable(const std::string& TableName, bool ifExists)
{
	int32_t nIsSucceed = -1;
	//删表
	if (ifExists == 0) nIsSucceed = mysql_query(m_mysql, ("drop table " + TableName).c_str());
	else if (ifExists == 1) nIsSucceed = mysql_query(m_mysql, ("drop table if exists " + TableName).c_str());

	return nIsSucceed;
}

CTable* CDataBase::OpenTableInterface(const std::string& TableName, bool AutoCommit)
{
	CTable* pTable = NULL;
	//返回0表示确实有这张表
	mysql_query(m_mysql, ("SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA='" + m_dbName + "' AND TABLE_NAME='" + TableName + "'").c_str());
	//结果中如果行数 == 1说明确实有这张表
	if (mysql_store_result(m_mysql)->row_count == 1)
	{
		//新开一个表接口并为他的内部建立一个连接线
		pTable = new CTable(m_pMysqlManager, this, TableName);
		//把数据库里的内部连接线重新再制造一个放进CTable内
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