#include <SDKDDKVer.h>
#include "CUser.h"
#include "CMysqlManager.h"
#include "CDataBase.h"
#include "mysql.h"

CUser::CUser(CMysqlManager *pMysqlManager, const std::string& IP, int32_t port, const std::string& User, const std::string& PassWord)
{
	//从外部导入管理者指针
	m_pMysqlManager = pMysqlManager;
	//先把新开的指针放到容器里
	m_pMysqlManager->m_listUser.push_back(this);

	m_ip = IP;
	m_port = port;
	m_user = User;
	m_passWord = PassWord;

	m_mysql = mysql_init(NULL);
}

void CUser::Close()
{
	//关闭类内连接线
	if (m_mysql != 0)
{
		mysql_close(m_mysql);
		m_mysql = 0;
	}
	//找到容器里这个指针并删除
	m_pMysqlManager->DeleteOne(this);
}

CDataBase* CUser::CreateDataBase(const std::string& NewdbName, bool ifExists)
{
	int32_t nIsSucceed = -1;
	//判断是否创建成功
	if (ifExists == 0)
	{
		nIsSucceed = mysql_query(m_mysql, ("create database " + NewdbName).c_str());
	}
	if (ifExists == 1)
	{
		nIsSucceed = mysql_query(m_mysql, ("create database if not exists " + NewdbName).c_str());
	}

	CDataBase* pDataBase = NULL;
	//如果成功则转到该数据库
	if (nIsSucceed == 0) pDataBase = OpenDataBaseInterface(NewdbName);
	return pDataBase;
}

int32_t CUser::DropDataBase(CDataBase *pDataBase)
{
	int32_t nIsSucceed = mysql_query(m_mysql, ("drop database " + pDataBase->m_dbName).c_str());
	if (nIsSucceed == 0) m_pMysqlManager->DeleteOne(pDataBase);
	//如果删除失败，返回失败值
	return nIsSucceed;
}

CDataBase* CUser::OpenDataBaseInterface(const std::string& dbName)
{
	bool bSucceed = 0;
	//连接这个数据库看是否能连接上，连接上表示确实有这个数据库，连接不上返回错误
	CDataBase* pDataBase = new CDataBase(&bSucceed, m_pMysqlManager, m_ip, m_port, m_user, m_passWord, dbName);
	//如果连接不上，返回0
	if (bSucceed == 0)
{
		//在连接失败时构造函数内部已经有delete this的操作，所以只需要重置成0即可
		pDataBase = 0;
	}
	return pDataBase;
}

void CUser::CloseDataBaseInterface(CDataBase *pDataBase)
{
	m_pMysqlManager->DeleteOne(pDataBase);
	return;
}