#include "SqlString.h"
#include "CStringManager/CStringManagerAPI.h"
#include <stdint.h>

std::string SqlString::insertString(const std::string& tableName, const std::string& fields)
{
	std::string insertStr = "insert into `%s`(%s) values(%s)";
	std::string fieldsTemp = fields;
	int32_t count = (int32_t)CStringManager::Replace(fieldsTemp, ",", "");
	std::string valueStr;
	while (count-- != 0)
	{
		valueStr += "?,";
	}
	valueStr += "?";
	return CStringManager::Format(insertStr.c_str(), tableName.c_str(), fields.c_str(), valueStr.c_str());
}

std::string SqlString::updateString(const std::string& tableName, const std::string& fields, const std::string& whereString)
{
	std::string fieldsTemp = fields;
	CStringManager::Replace(fieldsTemp, ",", "=?,");
	fieldsTemp.append("=?");
	std::string updateStr = (whereString == "") ? "update `%s` set %s" : "update `%s` set %s where %s";
	return CStringManager::Format(updateStr.c_str(), tableName.c_str(), fieldsTemp.c_str(), whereString.c_str());
}

std::string SqlString::deleteString(const std::string& tableName, const std::string& whereString)
{
	return CStringManager::Format("delete from `%s` where %s", tableName.c_str(), whereString.c_str());
}

std::string SqlString::selectString(const std::string& tableName, const std::string& selectFields, const std::string& whereString)
{
	if (whereString == "")
	{
		return CStringManager::Format("select %s from `%s`", selectFields.c_str(), tableName.c_str());
	}
	return CStringManager::Format("select %s from `%s` where %s", selectFields.c_str(), tableName.c_str(), whereString.c_str());
}

std::string SqlString::createTableString(const std::string& tableName, const std::vector<std::string>& fields)
{
	std::string createTableStr = "create table `" + tableName + "`(";
	uint32_t index = 0;
	bool btrue = true;
	while (btrue)
	{
		createTableStr += fields[index++];
		if (index == fields.size())
		{
			break;
		}
		createTableStr += ",";
	}
	createTableStr += ");";
	return createTableStr;
}

std::string SqlString::createTableIfNotExistString(const std::string& tableName, const std::vector<std::string>& fields)
{
	std::string createTableStr = "create table if not exists `" + tableName + "`(";
	uint32_t index = 0;
	bool btrue = true;
	while (btrue)
	{
		createTableStr += fields[index++];
		if (index == fields.size())
		{
			break;
		}
		createTableStr += ",";
	}
	createTableStr += ");";
	return createTableStr;
}

std::string SqlString::createDatabaseString(const std::string& databaseName)
{
	return "create database `" + databaseName + "`";
}

std::string SqlString::destroyTableString(const std::string& tableName)
{
	return "drop table `" + tableName + "`";
}

std::string SqlString::destroyDatabaseString(const std::string& databaseName)
{
	return "drop database `" + databaseName + "`";
}

std::string SqlString::clearTableString(const std::string& tableName)
{
	return "truncate table `" + tableName + "`";
}

std::string SqlString::strToDate(const std::string& date)
{
	return "STR_TO_DATE('" + date + "', '%Y-%m-%d')";
}

std::string SqlString::fieldStrToDate(const std::string& field)
{
	return "STR_TO_DATE(" + field + ", '%Y-%m-%d')";
}

std::string SqlString::sqlMin(const std::string& field)
{
	return "MIN(" + field + ")";
}

std::string SqlString::sqlMax(const std::string& field)
{
	return "MAX(" + field + ")";
}

std::string SqlString::selectAllTableName(const std::string& dbName)
{
	return "select table_name from information_schema.tables where table_schema='" + dbName + "'";
}