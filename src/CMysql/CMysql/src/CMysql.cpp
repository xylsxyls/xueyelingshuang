#include <SDKDDKVer.h>
#include "CMysql.h"
#include "CMysqlManager.h"
#include "CDataBase.h"
#include "CTable.h"
#include "CHostIP.h"
#include "CUser.h"
#include "mysql.h"

CMysql::CMysql(const std::string& IP, int32_t port, const std::string& User, const std::string& PassWord, const std::string& dbName)
{
	//��ʼ��������
	m_pMysqlManager = new CMysqlManager;

	//�ѳ�ʼ���Ĳ����������еļ�¼������
	m_ip = IP;
	m_port = port;
	m_user = User;
	m_passWord = PassWord;
	m_dbName = dbName;
}

CMysql::~CMysql()
{
	delete m_pMysqlManager;
}

//ֻ�ǽ�IP��port�ļ�¼�����¿�����
CHostIP* CMysql::OpenHostIPInterface()
{
	return OpenHostIPInterface(m_ip, m_port);
}

CHostIP* CMysql::OpenHostIPInterface(const std::string& IP, int32_t port)
{
	//�ڴ���ʱ�ڴ����г�Ա����������ʼ��
	return new CHostIP(m_pMysqlManager, IP, port);
}

CUser* CMysql::OpenUserInterface()
{
	return OpenUserInterface(m_user, m_passWord);
}

CUser* CMysql::OpenUserInterface(const std::string& User, const std::string& PassWord)
{
	//�ڴ���ʱ�ڴ����г�Ա����������ʼ��
	return new CUser(m_pMysqlManager, m_ip, m_port, User, PassWord);
}

CDataBase* CMysql::OpenDataBaseInterface()
{
	return OpenDataBaseInterface(m_dbName);
}

CDataBase* CMysql::OpenDataBaseInterface(const std::string& dbName, bool AutoCommit)
{
	bool bSucceed = 0;
	CDataBase* pDataBase = new CDataBase(&bSucceed, m_pMysqlManager, m_ip, m_port, m_user, m_passWord, dbName);
	if (bSucceed == 0) pDataBase = 0;
	return pDataBase;
}

CTable* CMysql::OpenTableInterface(const std::string& TableName, bool AutoCommit)
{
	CDataBase* pDataBase = OpenDataBaseInterface(m_dbName);
	//�ڴ���ʱ�ڴ����г�Ա����������ʼ��
	CTable* pTable = new CTable(m_pMysqlManager, pDataBase, TableName);

	//���¿���CDataBase��������߽�������
	MYSQL *IsSucceed = mysql_real_connect(pTable->m_pDataBase->m_mysql, m_ip.c_str(), m_user.c_str(), m_passWord.c_str(), m_dbName.c_str(), m_port, NULL, 0);
	//�������ʧ�����ͷţ����ǲ��ͷŹ����ߣ�������ֻ����CMysql���ͷ�ʱ�Ż��ͷ�
	if (IsSucceed == NULL)
	{
		delete pTable;
		pTable = 0;
	}
	pTable->Refresh();
	return pTable;
}

/*
void CMysql::SelectOtherIPDataBase(std::string IP,std::string User,std::string PassWord,std::string dbName,int32_t port)
{
if(MysqlSucceed != 1) return;

mysql_close(mysql);
mysql = mysql_init(NULL);
IsSucceed = mysql_real_connect(mysql,IP,User,PassWord,dbName,port,NULL,0);
//�������ֵ��ָ�룬����Ϊ��ʱ��˵��ִ�гɹ�
if(IsSucceed != 0)
{
ConnectParameter.IP = IP;
ConnectParameter.User = User;
ConnectParameter.PassWord = PassWord;
ConnectParameter.dbName = dbName;
ConnectParameter.port = port;
}
else MysqlSucceed = 0;
return;
}

void CMysql::SelectTable(std::string TableName)
{
if(MysqlSucceed != 1) return;

ConnectParameter.TableName = TableName;
return;
}*/

#include "CTableField.h"
#include "CCondition.h"
#include "CUpdate.h"
#include "CSelect.h"

int32_t main()
{
	//�򿪱�ӿ�
	CMysql mysql;
	CTable* pTable = mysql.OpenTableInterface("test", 1);
	std::string xxx = (*pTable)[0]["name"].toValue();

	//��
	//���ü�¼
	CRecord rec(pTable);
	rec["name"] = "e";
	rec["number"] = 5;
	//pTable->Add(&rec);

	//��������
	CCondition con;
	//����and
	con && (CTableField("test", "name") == "d");
	con || (CTableField("test", "name") == "e");
	con && (CTableField("test", "number") <= 5);
	con(0, 4);
	con["test.name"]--;
	//����or��������
	//con || (CTableField("User","ID") <= CTableField("Department","UserID"));
	//����not
	//!con;

	//ɾ
	//int32_t x = pTable->Delete(&con);

	//��
	CUpdate upd;
	upd["ID"] = 10005;
	pTable->UpdateRecord(&upd, &con);

	//��
	CSelect sel;
	sel("test")["name"];
	sel("test")["ID"];
	sel("Depart")["name"];
	CTable table = pTable->SelectRecord(&sel, &con);
	CTable *pTable1 = mysql.OpenTableInterface("Depart");
	std::string strr = (*pTable1)[0]["depart.name"].toValue();
	int32_t xxxx = table.size();
	std::string strDepartName = table[0]["depart.name"].toValue();
	int32_t nUserID = table[0]["User.ID"].toValue();

	//rec["User"].pTable->mapAttri["User"].Name = "123456";
	//�޸��ֶ�����
	rec["User"]->m_length = 256;
	pTable->Add(&rec);

	return 0;
}