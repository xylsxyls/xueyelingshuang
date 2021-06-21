#include "SQLite.h"
#include "SQLitePrepareStatement.h"
#include "SQLiteResultSet.h"
#include "CStringManager/CStringManagerAPI.h"
#include "sqlite3.h"
#include "SqliteDatabase.h"

SQLite::SQLite(const std::string& dbFilePath) :
m_dbFilePath(dbFilePath),
m_spDb(nullptr)
{
	
}

SQLite::~SQLite()
{
	close();
}

void SQLite::open(PragmaFlag pragmaFlag)
{
	if (m_spDb != nullptr)
	{
		return;
	}

	m_spDb.reset(new SqliteDatabase);
	int32_t result = sqlite3_open(m_dbFilePath.c_str(), &m_spDb->m_db);
	if (result != 0)
	{
		printf("open sqlite failed, result = %d\n", result);
		return;
	}

	char* errorMessage = nullptr;
	result = sqlite3_exec(m_spDb->m_db, CStringManager::Format("PRAGMA synchronous = %d;", (int32_t)pragmaFlag).c_str(), nullptr, nullptr, &errorMessage);
	if (result != 0)
	{
		printf("open init error, result = %d, errorMessage = %s\n", result, errorMessage);
		sqlite3_free(&errorMessage);
	}
}

void SQLite::close()
{
	if (m_spDb == nullptr)
	{
		return;
	}
	if (m_spDb->m_stmt != nullptr)
	{
		int32_t result = sqlite3_finalize(m_spDb->m_stmt);
		if (result != 0)
		{
			printf("~SqliteStmt sqlite3_finalize failed, result = %d\n", result);
		}
		m_spDb->m_stmt = nullptr;
		m_spDb->m_stepResult = 0;
		m_spDb->m_isBeginRead = false;
	}

	if (m_spDb->m_db != nullptr)
	{
		int32_t result = sqlite3_close(m_spDb->m_db);
		if (result != 0)
		{
			printf("sqlite3_close failed, result = %d\n", result);
		}
		m_spDb->m_db = nullptr;
	}
	m_spDb = nullptr;
}

bool SQLite::isOpen()
{
	return m_spDb != nullptr;
}

SQLitePrepareStatement SQLite::preparedCreator(const std::string& sqlString)
{
	SQLitePrepareStatement prepareQuery(m_spDb);
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
	if (m_spDb == nullptr)
	{
		return;
	}
	try
	{
		char* errorMessage = nullptr;
		int32_t result = sqlite3_exec(m_spDb->m_db, "begin transaction;", nullptr, nullptr, &errorMessage);
		if (result != 0)
		{
			printf("transaction failed, result = %d, errorMessage = %s\n", result, errorMessage);
			sqlite3_free(&errorMessage);
		}
	}
	catch (...)
	{
		
	}
}

void SQLite::commit()
{
	if (m_spDb == nullptr)
	{
		return;
	}
	try
	{
		char* errorMessage = nullptr;
		int32_t result = sqlite3_exec(m_spDb->m_db, "commit transaction;", nullptr, nullptr, &errorMessage);
		if (result != 0)
		{
			printf("commit failed, result = %d, errorMessage = %s\n", result, errorMessage);
			sqlite3_free(&errorMessage);
		}
	}
	catch (...)
	{

	}
}

void SQLite::rollback()
{
	if (m_spDb == nullptr)
	{
		return;
	}
	try
	{
		char* errorMessage = nullptr;
		int32_t result = sqlite3_exec(m_spDb->m_db, "rollback transaction;", nullptr, nullptr, &errorMessage);
		if (result != 0)
		{
			printf("rollback failed, result = %d, errorMessage = %s\n", result, errorMessage);
			sqlite3_free(&errorMessage);
		}
	}
	catch (...)
	{

	}
}

//#include "SqlString/SqlStringAPI.h"
//#include "CSystem/CSystemAPI.h"
//
//int main()
//{
//	
//	{
//		SQLite lite(CSystem::GetCurrentExePath() + "test.db");
//		lite.open();
//		std::vector<std::string> vecField;
//		vecField.push_back("id2 interger");
//		vecField.push_back("nAme varchar(20)");
//		std::string sqlString = SqlString::selectString("e123", "id2,name");
//		auto prepare = lite.preparedCreator(sqlString);
//		//prepare.setInt(0, 2);
//		//prepare.setString(1, "22");
//		//std::string setText = "a123456789012345678901111111111111111111111111111111111123";
//		//setText.push_back('\0');
//		//setText.append("1213");
//		//prepare.setString(1, setText);
//		//prepare.exec();
//		//prepare.reset();
//		//prepare.setString(1, "23");
//		//prepare.exec();
//
//		
//		SQLiteResultSet result = lite.execute(prepare);
//		//std::vector<std::vector<std::string>> vecResult = result.toVector();
//		int32_t rowCount = result.rowsCount();
//		rowCount = result.rowsCount();
//		int xxd = result.rowsAffected();
//		int x = result.getInt(0);
//		std::string xx = result.getString("name");
//		lite.close();
//		//lite.execute(prepare);
//		
//	}
//	//lite.close();
//	int x = 3;
//
//	//std::vector<std::string> vecField;
//	//vecField.push_back("id interger");
//	//vecField.push_back("name varchar(20)");
//	//std::string sqlString = SqlString::createTableString("d123", vecField);
//	//sqlite3* m_db = nullptr;
//	//sqlite3_open((CSystem::GetCurrentExePath() + "test.db").c_str(), &m_db);
//	//char* errorMessage = nullptr;
//	//sqlite3_exec(m_db, CStringManager::Format("PRAGMA synchronous = %d;", (int32_t)0).c_str(), nullptr, nullptr, &errorMessage);
//	//sqlite3_stmt* m_stmt = nullptr;
//	//int32_t result = sqlite3_prepare(m_db, sqlString.c_str(), sqlString.size(), &m_stmt, nullptr);
//	//sqlite3_step(m_stmt);
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