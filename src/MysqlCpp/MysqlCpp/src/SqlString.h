#pragma once
#include <string>
#include "MysqlCppMacro.h"

class MysqlCppAPI SqlString
{
public:
	static std::string insertString(const std::string& tableName, const std::string& fields);

	static std::string updateString(const std::string& tableName, const std::string& setString, const std::string& whereString = "");

	static std::string deleteString(const std::string& tableName, const std::string whereString);

	static std::string selectString(const std::string& tableName, const std::string whereString, const std::string& selectString = "*");
};