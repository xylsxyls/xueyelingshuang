#include <SDKDDKVer.h>
#include "CUser.h"
#include "CMysqlManager.h"
#include "CDataBase.h"
#include "mysql.h"

CUser::CUser(CMysqlManager *pMysqlManager, const std::string& IP, int32_t port, const std::string& User, const std::string& PassWord)
{
	//���ⲿ���������ָ��
	m_pMysqlManager = pMysqlManager;
	//�Ȱ��¿���ָ��ŵ�������
	m_pMysqlManager->m_listUser.push_back(this);

	m_ip = IP;
	m_port = port;
	m_user = User;
	m_passWord = PassWord;

	m_mysql = mysql_init(NULL);
}

void CUser::Close()
{
	//�ر�����������
	if (m_mysql != 0)
{
		mysql_close(m_mysql);
		m_mysql = 0;
	}
	//�ҵ����������ָ�벢ɾ��
	m_pMysqlManager->DeleteOne(this);
}

CDataBase* CUser::CreateDataBase(const std::string& NewdbName, bool ifExists)
{
	int32_t nIsSucceed = -1;
	//�ж��Ƿ񴴽��ɹ�
	if (ifExists == 0)
	{
		nIsSucceed = mysql_query(m_mysql, ("create database " + NewdbName).c_str());
	}
	if (ifExists == 1)
	{
		nIsSucceed = mysql_query(m_mysql, ("create database if not exists " + NewdbName).c_str());
	}

	CDataBase* pDataBase = NULL;
	//����ɹ���ת�������ݿ�
	if (nIsSucceed == 0) pDataBase = OpenDataBaseInterface(NewdbName);
	return pDataBase;
}

int32_t CUser::DropDataBase(CDataBase *pDataBase)
{
	int32_t nIsSucceed = mysql_query(m_mysql, ("drop database " + pDataBase->m_dbName).c_str());
	if (nIsSucceed == 0) m_pMysqlManager->DeleteOne(pDataBase);
	//���ɾ��ʧ�ܣ�����ʧ��ֵ
	return nIsSucceed;
}

CDataBase* CUser::OpenDataBaseInterface(const std::string& dbName)
{
	bool bSucceed = 0;
	//����������ݿ⿴�Ƿ��������ϣ������ϱ�ʾȷʵ��������ݿ⣬���Ӳ��Ϸ��ش���
	CDataBase* pDataBase = new CDataBase(&bSucceed, m_pMysqlManager, m_ip, m_port, m_user, m_passWord, dbName);
	//������Ӳ��ϣ�����0
	if (bSucceed == 0)
{
		//������ʧ��ʱ���캯���ڲ��Ѿ���delete this�Ĳ���������ֻ��Ҫ���ó�0����
		pDataBase = 0;
	}
	return pDataBase;
}

void CUser::CloseDataBaseInterface(CDataBase *pDataBase)
{
	m_pMysqlManager->DeleteOne(pDataBase);
	return;
}