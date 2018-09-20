#pragma once
#include <string>
#include <vector>
#include "SqlStringMacro.h"

class SqlStringAPI SqlString
{
public:
	static std::string insertString(const std::string& tableName, const std::string& fields);

	static std::string updateString(const std::string& tableName, const std::string& setString, const std::string& whereString = "");

	static std::string deleteString(const std::string& tableName, const std::string& whereString);

	static std::string selectString(const std::string& tableName, const std::string& selectFields = "*", const std::string& whereString = "");

	//fields÷–name varchar(20)
	static std::string createTableString(const std::string& tableName, const std::vector<std::string>& fields);

	static std::string createDatabaseString(const std::string& databaseName);

	static std::string destroyTableString(const std::string& tableName);

	static std::string destroyDatabaseString(const std::string& databaseName);

	static std::string clearTableString(const std::string& tableName);
};