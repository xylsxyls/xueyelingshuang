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
	enum PragmaFlag
	{
		/** �ȫ�ķ�ʽ��������
		*/
		PRAGMA_FULL = 2,

		/** Ӳ�̲�����ʱ���ݿ��ļ�Ҳ����
		*/
		PRAGMA_NORMAL = 1,

		/** û�б������ƣ����ǳ����������Ӱ���ļ�
		*/
		PRAGMA_OFF = 0
	};
public:
	/** ���캯��
	@param [in] dbName ���ݿ��ļ�·��
	@param [in] pragmaFlag ���ݿ�洢��ʽ
	*/
	SQLite(const std::string& dbFilePath, PragmaFlag pragmaFlag = PRAGMA_OFF);

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

private:
	QSqlDatabase* m_db;
};