#pragma once
#include "CMysqlMacro.h"
#include <string>
#include <stdint.h>

class CDataBase;
class CMysqlManager;
struct st_mysql;
typedef st_mysql MYSQL;
//IP���û�
//��CIP��������ʱ�������IP��port�ͱ��뱻��ʼ�����ڲ���������Ҳ���뱻��ʼ��
class CMysqlAPI CHostIP
{
public:
	CHostIP(CMysqlManager *pMysqlManager, const std::string& IP, int32_t port);

public:
	//ת����һ���û������ݿ�
	CDataBase* OpenUserDataBaseInterface(const std::string& User, const std::string& PassWord, const std::string& dbName);
	void CloseDataBaseInterface(CDataBase* pCDataBase);

public:
	//CIP�е����ݿ�������
	MYSQL* m_mysql;
	//ʹ��CMysql�еĹ����߽��й���
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