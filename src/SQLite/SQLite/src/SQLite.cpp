#include "SQLite.h"
#include <QSqlQuery>
#include "SQLitePrepareStatement.h"
#include "SQLiteResultSet.h"
#include <QStringList>
#include "CStringManager/CStringManagerAPI.h"
#include <QSqlError>

SQLite::SQLite(const std::string& dbFilePath) :
m_db(nullptr)
{
	QSqlDatabase::drivers();
	m_db = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "SQLite"));

	init(dbFilePath);
}

SQLite::~SQLite()
{
	free();
}

void SQLite::open(PragmaFlag pragmaFlag)
{
	if (m_db == nullptr)
	{
		return;
	}
	//打开数据库连接 调用 open() 方法打开数据库物理连接。在打开连接之前，连接不可用
	if (!m_db->open())
	{
		QString error = m_db->lastError().text();
		free();
		return;
	}
	m_db->exec(CStringManager::Format("PRAGMA synchronous = %d;", (int32_t)pragmaFlag).c_str());
}

void SQLite::close()
{
	if (m_db == nullptr)
	{
		return;
	}
	m_db->close();
}

bool SQLite::isOpen()
{
	if (m_db == nullptr)
	{
		return false;
	}
	return m_db->isOpen();
}

SQLitePrepareStatement SQLite::preparedCreator(const std::string& sqlString)
{
	SQLitePrepareStatement prepareQuery(m_db);
	prepareQuery.prepare(sqlString.c_str());
	return prepareQuery;
}

SQLiteResultSet SQLite::execute(SQLitePrepareStatement& spPrepareQuery)
{
	try
	{
		SQLiteResultSet spResult(spPrepareQuery);
		if (!spPrepareQuery.exec())
		{
			return SQLiteResultSet();
		}
		return spResult;
	}
	catch (...)
	{
		return SQLiteResultSet();
	}
}

void SQLite::transaction()
{
	if (m_db == nullptr)
	{
		return;
	}
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
	if (m_db == nullptr)
	{
		return;
	}
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
	if (m_db == nullptr)
	{
		return;
	}
	try
	{
		m_db->rollback();
	}
	catch (...)
	{

	}
}

void SQLite::init(const std::string& dbFilePath)
{
	if (m_db == nullptr)
	{
		return;
	}
	//数据库名
	m_db->setDatabaseName(QString::fromStdWString(CStringManager::AnsiToUnicode(dbFilePath)));
}

void SQLite::free()
{
	if (m_db != nullptr)
	{
		//释放数据库连接
		delete m_db;
		m_db = nullptr;
		QSqlDatabase::removeDatabase("SQLite");
	}
}

//int main()
//{
//	return 0;
//}

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