#include "MysqlCpp.h"
#include "CStringManager/CStringManagerAPI.h"
#include "MysqlCppPrepareStatement.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#include "mysql_connection.h"
#include "cppconn/statement.h"
#include "cppconn/resultset.h"
#include "cppconn/prepared_statement.h"
#include "mysql_driver.h"
#include "cppconn/driver.h"  
#include "cppconn/statement.h"  
#include "cppconn/metadata.h"  
#include "cppconn/exception.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#pragma comment(lib, "mysqlcppconn.lib")

MysqlCpp::MysqlCpp():
m_driver(nullptr),
m_con(nullptr)
{
	m_driver = sql::mysql::get_mysql_driver_instance();
}

MysqlCpp::~MysqlCpp()
{
	if (m_con != nullptr)
	{
		delete m_con;
		m_con = nullptr;
	}
}

bool MysqlCpp::connect(const std::string& ip,
					   int32_t port,
					   const std::string& account,
					   const std::string& password)
{
	if (m_driver == nullptr)
	{
		return false;
	}

	if (m_con != nullptr)
	{
		delete m_con;
		m_con = nullptr;
	}

	m_con = m_driver->connect(CStringManager::Format("tcp://%s:%d", ip.c_str(), port).c_str(), account.c_str(), password.c_str());
	if (m_con == nullptr)
	{
		return false;
	}
	return m_con->isValid();
}

void MysqlCpp::selectDb(const std::string& dbName, bool autoCommit)
{
	if (!check())
	{
		return;
	}
	m_con->setSchema(dbName.c_str());
	setAutoCommit(autoCommit);
}

std::shared_ptr<MysqlCppPrepareStatement> MysqlCpp::PreparedStatementCreator(const std::string& sqlString)
{
	try
	{
		return std::shared_ptr<MysqlCppPrepareStatement>(new MysqlCppPrepareStatement(m_con->prepareStatement(sqlString)));
	}
	catch (...)
	{
		return std::shared_ptr<MysqlCppPrepareStatement>(new MysqlCppPrepareStatement(nullptr));
	}
}

std::shared_ptr<MysqlCppResultSet> MysqlCpp::execute(const std::shared_ptr<MysqlCppPrepareStatement>& statement)
{
	if (!check())
	{
		return std::shared_ptr<MysqlCppResultSet>(new MysqlCppResultSet(nullptr));
	}
	try
	{
		auto preparedStatement = statement->preparedStatement();
		if (preparedStatement == nullptr)
		{
			return std::shared_ptr<MysqlCppResultSet>(new MysqlCppResultSet(nullptr));
		}
		sql::ResultSet* result = preparedStatement->executeQuery();
		result != nullptr ? commit() : rollback();
		return std::shared_ptr<MysqlCppResultSet>(new MysqlCppResultSet(result));
	}
	catch (...)
	{
		rollback();
		return std::shared_ptr<MysqlCppResultSet>(new MysqlCppResultSet(nullptr));
	}
	rollback();
	return std::shared_ptr<MysqlCppResultSet>(new MysqlCppResultSet(nullptr));
}

void MysqlCpp::setAutoCommit(bool autoCommit)
{
	if (!check())
	{
		return;
	}
	m_con->setAutoCommit(autoCommit);
}

void MysqlCpp::commit()
{
	if (!check())
	{
		return;
	}
	m_con->commit();
}

void MysqlCpp::rollback()
{
	if (!check())
	{
		return;
	}
	m_con->rollback();
}

bool MysqlCpp::check()
{
	return m_driver != nullptr && m_con != nullptr;
}

//int main()
//{
//	MysqlCpp mysql;
//	bool isConnect = mysql.connect("127.0.0.1", 3306, "root", "");
//	mysql.selectDb("test");
//	auto statement = mysql.PreparedStatementCreator(SqlString::selectString("testtable", "id=3","num"));
//	//statement->setInt(1, 80);
//	//statement->setString(2, "aaa");
//	//statement->setDouble(3, 808.22);
//	std::shared_ptr<MysqlCppResultSet> result = mysql.executeResult(statement);
//	while (result->next())
//	{
//		int id = result->getInt("num");
//		id = id;
//	}
//	getchar();
//	return 0;
//}