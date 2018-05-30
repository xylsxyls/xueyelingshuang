#pragma once
#include "CMysqlMacro.h"
#include <string>
#include <stdint.h>

class CMysqlManager;
class CHostIP;
class CUser;
class CDataBase;
class CTable;
//��ɾ����֮����������ת����һ�ű��ٽ��в���������ᱨ��
//bool ifExists = 1����ʾĬ�ϱض����������٣�����ֶ���ֵ0��������Ƿ���ڵ��жϣ����������������Ӧ����ֵ
//CIP��CUser��CDataBase�����ж������������Կ��û��������ݿ⣬����ÿ��������һ�����ӣ�����CIP��������ר�����ڿ��û�
//int����ֵ����������ݿ���ķ���ֵ��������󷵻ش���ֵ
class CMysqlAPI CMysql
{
public:
	CMysql(const std::string& IP = "127.0.0.1", int32_t port = 3306, const std::string& User = "root", const std::string& PassWord = "", const std::string& dbName = "test");
	~CMysql();

public:
	//��IP�Ľӿڣ�����ʼ��CIP�����ڹ���User
	CHostIP* OpenHostIPInterface();
	CHostIP* OpenHostIPInterface(const std::string& IP, int32_t port = 3306);
	//��User�Ľӿڣ����ڹ������ݿ�
	CUser* OpenUserInterface();
	CUser* OpenUserInterface(const std::string& User, const std::string& PassWord);
	//�����ݿ�ӿڣ����ڹ����
	CDataBase* OpenDataBaseInterface();
	CDataBase* OpenDataBaseInterface(const std::string& dbName, bool AutoCommit = 1);
	//�򿪱�ӿڣ��򿪵�ʱ���Ѹñ��������Ϣȫ���ŵ��ڴ��У��������ȫ��ȡ��������ʹ�����ݿ�ӿ�
	CTable* OpenTableInterface(const std::string& TableName, bool AutoCommit = 1);

public:
	//ת����һ��IP�����ݿ�
	//void SelectOtherIPDataBase(std::string IP,std::string User,std::string PassWord,std::string dbName,int32_t port = 3306);

	//ѡ�����в���
	//void SelectTable(std::string TableName);

public:
	//����������õ���ͬһ�����������ݿ���һ����һ���ߣ�������Ҫ�ֿ�����
	//CMysql�Ĺ����ߣ������ڴ����ͺ�������
	CMysqlManager *m_pMysqlManager;
	//��¼���ڵ�port
	int32_t m_port;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//��¼�������ڵ�IP
	std::string m_ip;
	//��¼���ڵ�User
	std::string m_user;
	//��¼���ڵ�����
	std::string m_passWord;
	//��¼���ڵ����ݿ���
	std::string m_dbName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

