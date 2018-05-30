#pragma once
#include "CMysqlMacro.h"
#include <string>
#include <stdint.h>

class CMysqlManager;
class CDataBase;
struct st_mysql;
typedef st_mysql MYSQL;
//�û������ݿ�
//CUser������ʱ�����ʼ�����г�Ա����
class CMysqlAPI CUser
{
public:
	CUser(CMysqlManager *pMysqlManager, const std::string& IP, int32_t port, const std::string& User, const std::string& PassWord);
	void Close();

public:

	//�������ݿ⣬����1��ʾ�ɹ���ifExists = 1��ʾ���۴治���ڶ�������ifExists = 0��ʾ������������򴴽��������򷵻ش���
	//����֮���Զ�ת��������ݿ�
	CDataBase* CreateDataBase(const std::string& NewdbName, bool ifExists = 1);
	//�������ݿ⣬ͬʱ�ͷ�pDataBase�ռ�
	int32_t DropDataBase(CDataBase *pDataBase);
	//����Ҫ���������ݿ�
	CDataBase* OpenDataBaseInterface(const std::string& dbName);
	//�ر����ݿ�
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