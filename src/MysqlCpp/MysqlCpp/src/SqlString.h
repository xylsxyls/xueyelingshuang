#pragma once
#include <string>
#include "MysqlCppMacro.h"
#include <vector>

class MysqlCppAPI SqlString
{
public:
	static std::string insertString(const std::string& tableName, const std::string& fields);

	static std::string updateString(const std::string& tableName, const std::string& setString, const std::string& whereString = "");

	static std::string deleteString(const std::string& tableName, const std::string whereString);

	static std::string selectString(const std::string& tableName, const std::string& selectFields = "*", const std::string whereString = "");

	//fields÷–name varchar(20)
	static std::string createTableString(const std::string& tableName, const std::vector<std::string>& fields);

	static std::string createDatabaseString(const std::string& databaseName);

	static std::string destroyTableString(const std::string& tableName);

	static std::string destroyDatabaseString(const std::string& databaseName);

	static std::string clearTableString(const std::string& tableName);

	static std::string importString(const std::string sqlPath,
									const std::string& host,
									const std::string& user,
									const std::string& password,
									const std::string& database);

	static std::string exportString(const std::string sqlPath,
									const std::string& host,
									const std::string& user,
									const std::string& password,
									const std::string& exportData);
};