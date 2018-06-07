#pragma once
#include <stdint.h>
#include <string>
#include "MysqlCppMacro.h"
#include <vector>

namespace sql
{
	class ResultSet;
}

/** 该类等同于sql::ResultSet
*/
class MysqlCppAPI MysqlCppResultSet
{
public:
	/** 构造函数
	*/
	MysqlCppResultSet(sql::ResultSet* resultSet);

	/** 析构函数
	*/
	~MysqlCppResultSet();

public:
	std::vector<std::vector<std::string>> toVector();

	int32_t columnCount();

public:
	std::istream* getBlob(uint32_t columnIndex) const;
	std::istream* getBlob(const std::string& columnLabel) const;

	bool getBoolean(uint32_t columnIndex) const;
	bool getBoolean(const std::string& columnLabel) const;

	long double getDouble(uint32_t columnIndex) const;
	long double getDouble(const std::string& columnLabel) const;

	int32_t getInt(uint32_t columnIndex) const;
	int32_t getInt(const std::string& columnLabel) const;

	uint32_t getUInt(uint32_t columnIndex) const;
	uint32_t getUInt(const std::string& columnLabel) const;

	int64_t getInt64(uint32_t columnIndex) const;
	int64_t getInt64(const std::string& columnLabel) const;

	uint64_t getUInt64(uint32_t columnIndex) const;
	uint64_t getUInt64(const std::string& columnLabel) const;

	std::string getString(uint32_t columnIndex) const;
	std::string getString(const std::string& columnLabel) const;

	bool next();
	bool previous();

	size_t rowsCount() const;

private:
	sql::ResultSet* m_resultSet;
};