#include "SqlString.h"
#include "CStringManager/CStringManagerAPI.h"
#include <stdint.h>


std::string SqlString::insertString(const std::string& tableName, const std::string& fields)
{
	std::string insertStr = "insert into %s(%s) values(%s)";
	std::string fieldsTemp = fields;
	int32_t count = CStringManager::Replace(fieldsTemp, ",", "");
	std::string valueStr;
	while (count-- != 0)
	{
		valueStr += "?,";
	}
	valueStr += "?";
	return CStringManager::Format(insertStr.c_str(), tableName.c_str(), fields.c_str(), valueStr.c_str());
}

std::string SqlString::updateString(const std::string& tableName, const std::string& setString, const std::string& whereString)
{
	std::string updateStr = (whereString == "") ? "update `%s` set %s" : "update `%s` set %s where %s";
	return CStringManager::Format(updateStr.c_str(), tableName.c_str(), setString.c_str(), whereString.c_str());
}

std::string SqlString::deleteString(const std::string& tableName, const std::string whereString)
{
	return CStringManager::Format("delete from %s where %s", tableName.c_str(), whereString.c_str());
}

std::string SqlString::selectString(const std::string& tableName, const std::string whereString, const std::string& selectFields)
{
	return CStringManager::Format("select %s from %s where %s", selectFields.c_str(), tableName.c_str(), whereString.c_str());
}

std::string SqlString::createTableString(const std::string& tableName, const std::vector<std::string>& fields)
{
	std::string createTableStr = "create table " + tableName + "(";
	int32_t index = -1;
	while (true)
	{
		createTableStr += fields[++index];
		if (index == fields.size() - 1)
		{
			break;
		}
		createTableStr += ",";
	}
	createTableStr += ");";
	return createTableStr;
}

std::string SqlString::createDatabase(const std::string& databaseName)
{
	return "CREATE DATABASE " + databaseName;
}
