#pragma once
#include "SQLiteMacro.h"
#include <string>
#include <memory>

struct SqliteDatabase;
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
	*/
	SQLite(const std::string& dbFilePath);

	/** ��������
	*/
	~SQLite();

public:
	/** �����ݿ�
	@param [in] pragmaFlag ���ݿ�洢��ʽ
	*/
	void open(PragmaFlag pragmaFlag = PRAGMA_OFF);

	/** �ر����ݿ�
	*/
	void close();

	/** ���ݿ��Ƿ��Ѿ���
	@return �����Ƿ��Ѿ���
	*/
	bool isOpen();

	/** ����һ��������ɾ�Ĳ����
	@param [in] sqlString sql�ַ������ֶγ��Ȳ�������
	@return ���ز�������������
	*/
	SQLitePrepareStatement preparedCreator(const std::string& sqlString);

	/** ִ�����
	@param [in] spPrepareQuery creator��������
	@return ����ִ�н��	
	*/
	SQLiteResultSet execute(SQLitePrepareStatement& spPrepareQuery);

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
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_dbFilePath;
	std::shared_ptr<SqliteDatabase> m_spDb;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};