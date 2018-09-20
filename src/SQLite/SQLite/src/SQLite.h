#pragma once
#include "SQLiteMacro.h"
#include <string>
#include <memory>

class QSqlDatabase;
class SQLitePrepareStatement;
class SQLiteResultSet;

class SQLiteAPI SQLite
{
public:
	/** ���캯��
	@param [in] dbName ���ݿ��ļ�·��
	*/
	SQLite(const std::string& dbFilePath);

	/** ��������
	*/
	~SQLite();

public:
	/** ����һ��������ɾ�Ĳ����
	@param [in] sqlString sql�ַ���
	@return ���ز�������������
	*/
	std::shared_ptr<SQLitePrepareStatement> preparedCreator(const std::string& sqlString);

	/** ִ�����
	@param [in] spPrepareQuery creator��������
	@return ����ִ�н��	
	*/
	std::shared_ptr<SQLiteResultSet> execute(const std::shared_ptr<SQLitePrepareStatement>& spPrepareQuery);

	/** ��������
	*/
	void transaction();

	/** �ύ
	*/
	void commit();

	/** �ع�
	*/
	void rollback();

	void run();

private:
	QSqlDatabase* m_db;
};