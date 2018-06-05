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

	static std::string selectString(const std::string& tableName, const std::string whereString, const std::string& selectString = "*");

	//fields÷–name varchar(20)
	static std::string createTableString(const std::string& tableName, const std::vector<std::string>& fields);

	static std::string createDatabase(const std::string& databaseName);
};