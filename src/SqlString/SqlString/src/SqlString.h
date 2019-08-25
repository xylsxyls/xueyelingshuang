#pragma once
#include <string>
#include <vector>
#include "SqlStringMacro.h"

class SqlStringAPI SqlString
{
public:
	static std::string insertString(const std::string& tableName, const std::string& fields);

	//fields逗号分隔，生成的字符串有占位符需要填充
	static std::string updateString(const std::string& tableName, const std::string& fields, const std::string& whereString = "");

	static std::string deleteString(const std::string& tableName, const std::string& whereString);

	static std::string selectString(const std::string& tableName, const std::string& selectFields = "*", const std::string& whereString = "");

	//fields中name varchar(20)
	static std::string createTableString(const std::string& tableName, const std::vector<std::string>& fields);

	//fields中name varchar(20)
	static std::string createTableIfNotExistString(const std::string& tableName, const std::vector<std::string>& fields);

	static std::string createDatabaseString(const std::string& databaseName);

	static std::string destroyTableString(const std::string& tableName);

	static std::string destroyDatabaseString(const std::string& databaseName);

	static std::string clearTableString(const std::string& tableName);

	static std::string strToDate(const std::string& date);

	static std::string fieldStrToDate(const std::string& field);

	static std::string sqlMin(const std::string& field);

	static std::string sqlMax(const std::string& field);

	static std::string selectAllTableName(const std::string& dbName);
};