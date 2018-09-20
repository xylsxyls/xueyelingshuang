#include "SQLite.h"
#include <QSqlQuery>
#include "SQLitePrepareStatement.h"
#include "SQLiteResultSet.h"

SQLite::SQLite(const std::string& dbFilePath) :
m_db(nullptr)
{
	//QSqlDatabase::drivers();
	m_db = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));

	//数据库名
	m_db->setDatabaseName(dbFilePath.c_str());

	//打开数据库连接 调用 open() 方法打开数据库物理连接。在打开连接之前，连接不可用
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
		//释放数据库连接
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
	//数据库名
	//m_db->setDatabaseName("../info2.db");
	//m_db->open();
	
	
	//query.exec("create table if not exists student(id int primary key, name varchar(255), age int, score int)");
	//query.prepare("insert into student(id,name, age, score) values(:id,:name, :age, :score)");
	////给字段设置内容 list
	//QVariantList idList;
	//idList << 1 << 2 << 3;
	//QVariantList nameList;
	//nameList << "xiaoming" << "xiaolong" << "xiaojiang";
	//QVariantList ageList;
	//ageList << 11 << 22 << 33;
	//QVariantList scoreList;
	//scoreList << 59 << 69 << 70;
	////给字段绑定相应的值 按顺序绑定
	//query.addBindValue(idList);
	//query.addBindValue(nameList);
	//query.addBindValue(ageList);
	//query.addBindValue(scoreList);
	////执行预处理命令
	
	

	//query->exec("select * from tablests");
	//while (query->next())
	//{
	//	//取出当前行的内容
	//	qDebug() << query->value(0).toInt()
	//		<< query->value(1).toString()
	//		<< query->value("age").toInt()
	//		<< query->value("score").toInt();
	//}
}
