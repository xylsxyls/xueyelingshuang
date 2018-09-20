#pragma once
#include "MysqlCppMacro.h"
#include "MysqlCppResultSet.h"
#include "MysqlCppPrepareStatement.h"
#include <memory>

namespace sql
{
	class Driver;
	class Connection;
}

class MysqlCppPrepareStatement;

class MysqlCppAPI MysqlCpp
{
public:
	/** ���캯��
	*/
	MysqlCpp();

	/** ��������
	*/
	~MysqlCpp();

public:
	/** �������ݿ�
	@param [in] ip IP��ַ
	@param [in] port �˿�
	@param [in] account �˺�
	@param [in] password ����
	@param [in] dbName ����
	@return ���������Ƿ�ɹ�
	*/
	bool connect(const std::string& ip,
				 int32_t port,
				 const std::string& account,
				 const std::string& password);

	/** ѡ�����ݿ�
	@param [in] dbName ���ݿ���
	@param [in] autoCommit �Ƿ��Զ��ύ
	*/
	void selectDb(const std::string& dbName, bool autoCommit = false);

	/** ����һ��������ɾ�Ĳ����
	@param [in] sqlString sql�ַ���
	@return ���ز�������������
	*/
	std::shared_ptr<MysqlCppPrepareStatement> PreparedStatementCreator(const std::string& sqlString);

	/** ������
	@param [in] statement �������
	@return ���ز�ѯ���
	*/
	std::shared_ptr<MysqlCppResultSet> execute(const std::shared_ptr<MysqlCppPrepareStatement>& statement);

	/** ����
	@param [in] sqlPath �����sql�ļ�·��
	@param [in] host ����IP��ַ
	@param [in] user �û���
	@param [in] password ����
	@param [in] database ���ݿ���
	*/
	void importSql(const std::string sqlPath,
				   const std::string& host,
				   const std::string& user,
				   const std::string& password,
				   const std::string& database);

	/** ����
	@param [in] sqlPath �����sql�ļ�·��
	@param [in] host ����IP��ַ
	@param [in] user �û���
	@param [in] password ����
	@param [in] exportData �������ݣ�database����database table
	*/
	void exportSql(const std::string sqlPath,
				   const std::string& host,
				   const std::string& user,
				   const std::string& password,
				   const std::string& exportData);

	/** �����Ƿ��Զ��ύ
	@param [in] autoCommit �Ƿ��Զ��ύ
	*/
	void setAutoCommit(bool autoCommit);

	/** �ύ
	*/
	void commit();

	/** �ع�
	*/
	void rollback();

protected:
	bool check();

public:
	sql::Driver* m_driver;
	sql::Connection* m_con;
};