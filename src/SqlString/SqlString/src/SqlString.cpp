#include "SqlString.h"
#include "CStringManager/CStringManagerAPI.h"
#include <algorithm>
#include <stdint.h>

std::string SqlString::insertString(const std::string& tableName, const std::string& fields)
{
    if (tableName.find('\0') != std::string::npos || fields.find('\0') != std::string::npos)
    {
        return std::string();
    }
    const size_t commas = static_cast<size_t>(std::count(fields.begin(), fields.end(), ','));
    std::string statement;
    statement.reserve(tableName.size() + fields.size() + commas * 2 + 28);
    statement.append("insert into `").append(tableName.c_str()).append("`(").append(fields.c_str()).append(") values(?");
    for (size_t index = 0; index < commas; ++index)
    {
        statement.append(",?");
    }
    statement.push_back(')');
    return statement;
}

std::string SqlString::updateString(const std::string& tableName, const std::string& fields, const std::string& whereString)
{
    if (tableName.find('\0') != std::string::npos || fields.find('\0') != std::string::npos ||
        whereString.find('\0') != std::string::npos)
    {
        return std::string();
    }
	std::string fieldsTemp = fields;
	CStringManager::Replace(fieldsTemp, ",", "=?,");
	fieldsTemp.append("=?");
	std::string updateStr = (whereString == "") ? "update `%s` set %s" : "update `%s` set %s where %s";
	return CStringManager::Format(updateStr.c_str(), tableName.c_str(), fieldsTemp.c_str(), whereString.c_str());
}

std::string SqlString::deleteString(const std::string& tableName, const std::string& whereString)
{
    if (tableName.find('\0') != std::string::npos || whereString.find('\0') != std::string::npos)
    {
        return std::string();
    }
	return CStringManager::Format("delete from `%s` where %s", tableName.c_str(), whereString.c_str());
}

std::string SqlString::selectString(const std::string& tableName, const std::string& selectFields, const std::string& whereString)
{
    if (tableName.find('\0') != std::string::npos || selectFields.find('\0') != std::string::npos ||
        whereString.find('\0') != std::string::npos)
    {
        return std::string();
    }
	if (whereString == "")
	{
		return CStringManager::Format("select %s from `%s`", selectFields.c_str(), tableName.c_str());
	}
	return CStringManager::Format("select %s from `%s` where %s", selectFields.c_str(), tableName.c_str(), whereString.c_str());
}

/** 拼接调用方提供的表标识符与列定义
@param [in] prefix 含 CREATE TABLE 与起始反引号的前缀
@param [in] tableName 可信表名
@param [in] fields 各列的完整定义
@return SQL 文本；空列列表返回空串
*/
static std::string BuildCreateTable(const char* prefix, const std::string& tableName, const std::vector<std::string>& fields)
{
    if (fields.empty())
    {
        return std::string();
    }
    std::string statement = prefix + tableName + "`(" + fields.front();
    for (size_t index = 1; index < fields.size(); ++index)
    {
        statement += "," + fields[index];
    }
    return statement + ");";
}

std::string SqlString::createTableString(const std::string& tableName, const std::vector<std::string>& fields)
{
    return BuildCreateTable("create table `", tableName, fields);
}

std::string SqlString::createTableIfNotExistString(const std::string& tableName, const std::vector<std::string>& fields)
{
    return BuildCreateTable("create table if not exists `", tableName, fields);
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

// 使用示例开始。单独编译本文件时可启用下方的 main。
//#include <iostream>
//int main()
//{
//    const std::string table = "reading";
//    const std::vector<std::string> columns = { "id INT PRIMARY KEY", "score INT" };
//    const std::vector<std::string> statements = {
//        SqlString::createDatabaseString("example"), SqlString::selectAllTableName("example"),
//        SqlString::createTableString(table, columns), SqlString::createTableIfNotExistString(table, columns),
//        SqlString::insertString(table, "id,score"), SqlString::updateString(table, "score", "id=?"),
//        SqlString::selectString(table, SqlString::sqlMin("score") + "," + SqlString::sqlMax("score"), "id=?"),
//        SqlString::strToDate("2026-11-20"), SqlString::fieldStrToDate("created_on"),
//        SqlString::deleteString(table, "id=?"), SqlString::clearTableString(table),
//        SqlString::destroyTableString(table), SqlString::destroyDatabaseString("example")
//    };
//    for (const auto& sql : statements)
//    {
//        std::cout << sql << std::endl;
//    }
//    return statements[4] == "insert into `reading`(id,score) values(?,?)" ? 0 : 1;
//}
// 使用示例结束。