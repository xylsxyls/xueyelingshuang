#pragma once
#include <string>
#include "CMysqlMacro.h"
#include <stdint.h>

struct st_mysql;
typedef st_mysql MYSQL;
class CTable;
class CMysqlManager;
//���ݿ�ܱ�
class CMysqlAPI CDataBase
{
public:
	//������ݿ�����ʧ������Զ��ͷţ��������������ָ���ͷţ���bSucceed��ʶ������new��������Ȼ��ֵ����Ҫ���ⲿ��0
	CDataBase(bool* bSucceed, CMysqlManager* pMysqlManager, const std::string& ip, int32_t port, const std::string& user, const std::string& password, const std::string& dbName);

public:
	//����
	CTable* CreateTable(const std::string& TableName, const std::string& FieldName, int32_t type, int32_t length, bool ifExists = 1, bool AutoCommit = 1);
	//ɾ��
	int32_t DropTable(const std::string& TableName, bool ifExists = 1);
	//�򿪱�ӿ�
	CTable* OpenTableInterface(const std::string& TableName, bool AutoCommit = 1);
	//�رձ�ӿ�
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