#include "SQLite.h"
#include <QSqlQuery>
#include "SQLitePrepareStatement.h"
#include "SQLiteResultSet.h"

SQLite::SQLite(const std::string& dbFilePath) :
m_db(nullptr)
{
	//QSqlDatabase::drivers();
	m_db = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));

	//���ݿ���
	m_db->setDatabaseName(dbFilePath.c_str());

	//�����ݿ����� ���� open() ���������ݿ��������ӡ��ڴ�����֮ǰ�����Ӳ�����
	if (!m_db->open())
	{
		delete m_db;
		m_db = nullptr;
	}
}

SQLite::~SQLite()
{
	if (m_db != nullptr)
	{
		//�ͷ����ݿ�����
		m_db->close();
		delete m_db;
		m_db = nullptr;
	}
}

std::shared_ptr<SQLitePrepareStatement> SQLite::preparedCreator(const std::string& sqlString)
{
	std::shared_ptr<SQLitePrepareStatement> spPrepareQuery;
	QSqlQuery* query = new QSqlQuery;
	if (query == nullptr)
	{
		return spPrepareQuery;
	}
	std::shared_ptr<QSqlQuery> spSqlQuery;
	spSqlQuery.reset(query);
	spSqlQuery->prepare(sqlString.c_str());
	spPrepareQuery.reset(new SQLitePrepareStatement(spSqlQuery));
	return spPrepareQuery;
}

std::shared_ptr<SQLiteResultSet> SQLite::execute(const std::shared_ptr<SQLitePrepareStatement>& spPrepareQuery)
{
	try
	{
		transaction();
		if (!spPrepareQuery->exec())
		{
			rollback();
			return std::shared_ptr<SQLiteResultSet>();
		}
		commit();
		std::shared_ptr<SQLiteResultSet> spResult;
		spResult.reset(new SQLiteResultSet(spPrepareQuery->sqlQuery()));
		return spResult;
	}
	catch (...)
	{
		rollback();
		return false;
	}
}

void SQLite::transaction()
{
	try
	{
		m_db->transaction();
	}
	catch (...)
	{
		
	}
}

void SQLite::commit()
{
	try
	{
		m_db->commit();
	}
	catch (...)
	{

	}
}

void SQLite::rollback()
{
	try
	{
		m_db->rollback();
	}
	catch (...)
	{

	}
}

void SQLite::run()
{
	//���ݿ���
	//m_db->setDatabaseName("../info2.db");
	//m_db->open();
	
	
	//query.exec("create table if not exists student(id int primary key, name varchar(255), age int, score int)");
	//query.prepare("insert into student(id,name, age, score) values(:id,:name, :age, :score)");
	////���ֶ��������� list
	//QVariantList idList;
	//idList << 1 << 2 << 3;
	//QVariantList nameList;
	//nameList << "xiaoming" << "xiaolong" << "xiaojiang";
	//QVariantList ageList;
	//ageList << 11 << 22 << 33;
	//QVariantList scoreList;
	//scoreList << 59 << 69 << 70;
	////���ֶΰ���Ӧ��ֵ ��˳���
	//query.addBindValue(idList);
	//query.addBindValue(nameList);
	//query.addBindValue(ageList);
	//query.addBindValue(scoreList);
	////ִ��Ԥ��������
	
	

	//query->exec("select * from tablests");
	//while (query->next())
	//{
	//	//ȡ����ǰ�е�����
	//	qDebug() << query->value(0).toInt()
	//		<< query->value(1).toString()
	//		<< query->value("age").toInt()
	//		<< query->value("score").toInt();
	//}
}
