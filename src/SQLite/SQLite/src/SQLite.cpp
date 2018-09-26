#include "SQLite.h"
#include <QSqlQuery>
#include "SQLitePrepareStatement.h"
#include "SQLiteResultSet.h"
#include <QStringList>
#include "CStringManager/CStringManagerAPI.h"

SQLite::SQLite(const std::string& dbFilePath, PragmaFlag pragmaFlag) :
m_db(nullptr)
{
	QSqlDatabase::drivers();
	m_db = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE"));

	//数据库名
	m_db->setDatabaseName(dbFilePath.c_str());

	//打开数据库连接 调用 open() 方法打开数据库物理连接。在打开连接之前，连接不可用
	if (!m_db->open())
	{
		delete m_db;
		m_db = nullptr;
	}
	m_db->exec(CStringManager::Format("PRAGMA synchronous = %d;", pragmaFlag).c_str());
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
		if (!spPrepareQuery->exec())
		{
			return std::shared_ptr<SQLiteResultSet>();
		}
		std::shared_ptr<SQLiteResultSet> spResult;
		spResult.reset(new SQLiteResultSet(spPrepareQuery->sqlQuery()));
		return spResult;
	}
	catch (...)
	{
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

//void SQLite::run()
//{
	//数据库名
	//QSqlDatabase* m_db = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE2"));
	//m_db->setDatabaseName("../info2.db");
	//m_db->open();
	////m_db->transaction();
	//QSqlQuery* query = new QSqlQuery(*m_db);
	//query->exec("create table if not exists student(id int, name varchar(255), age int, score int)");
	////m_db->commit();
	//
	//int count = 100;
	//while (count-- != 0)
	//{
	//	query->exec(CStringManager::Format("insert into student(id, name, age, score) values(%d,'1',32,33)", count).c_str());
	//	//query->bindValue(0, count);
	//	//query->bindValue(1, "1");
	//	//query->bindValue(2, 32);
	//	//query->bindValue(3, 33);
	//	//m_db->transaction();
	//	//query->exec();
	//	//query->exec(QString("delete from `student` where id=%1").arg(count));
	//	//m_db->commit();
	//}
	
	
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


	//QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
	//db.setDatabaseName("./info2.db");
	//if (!db.open())
	//	return;
	//
	//
	//QString create_sql = "create table if not exists student(id int);";
	//std::string insert_sql = "insert into student(id) values(%1);";
	//std::string insert_sql2 = "insert into student(id) values(%d);";
	//QString select_sql = "SELECT * FROM student;";
	////QSqlQuery dbOperator(db);
	//db.exec(create_sql);
	//
	//int begin = ::GetTickCount();
	//
	//db.exec("PRAGMA synchronous = 0;");
	////db.transaction();
	//for (int i = 0; i < 10000; i++)
	//{
	//	db.exec(QString(insert_sql.c_str()).arg(i));
	//}
	////db.commit();
	//RCSend("qt2 = %d", ::GetTickCount() - begin);
	//
	////sqlite3* sqlite3Handle = NULL;
	////sqlite3_open("./original.db", &sqlite3Handle);
	////sqlite3_exec(sqlite3Handle, create_sql.toUtf8().data(), sqlite3_callback_t, 0, 0);
	////for (int i = 0; i < 100; i++)
	////{
	////	sqlite3_exec(sqlite3Handle, CStringManager::Format(insert_sql2.c_str(), i).c_str(), sqlite3_callback_t, 0, 0);
	////}
	////
	////RCSend("qt = %d", ::GetTickCount() - begin);

//}