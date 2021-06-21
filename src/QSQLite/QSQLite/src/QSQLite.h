#pragma once
#include "QSQLiteMacro.h"
#include <string>
#include <memory>

class QSqlDatabase;
class QSQLitePrepareStatement;
class QSQLiteResultSet;

class QSQLiteAPI QSQLite
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
	QSQLite(const std::string& dbFilePath);

	/** ��������
	*/
	~QSQLite();

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
	@param [in] sqlString sql�ַ���
	@return ���ز�������������
	*/
	QSQLitePrepareStatement preparedCreator(const std::string& sqlString);

	/** ִ�����
	@param [in] spPrepareQuery creator��������
	@return ����ִ�н��	
	*/
	QSQLiteResultSet execute(QSQLitePrepareStatement& spPrepareQuery);

	/** ��������
	*/
	void transaction();

	/** �ύ
	*/
	void commit();

	/** �ع�
	*/
	void rollback();

protected:
	void init(const std::string& path);
	void free();

private:
	QSqlDatabase* m_db;
};