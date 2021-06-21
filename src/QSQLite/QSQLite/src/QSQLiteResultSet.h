#pragma once
#include "QSQLiteMacro.h"
#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

class QSqlQuery;
class QSQLitePrepareStatement;

class QSQLiteAPI QSQLiteResultSet
{
public:
	/** 构造函数
	*/
	QSQLiteResultSet();

	/** 构造函数
	@param [in] prepareStatement sql语句管理类
	*/
	QSQLiteResultSet(QSQLitePrepareStatement& prepareStatement);

public:
	std::vector<std::vector<std::string>> toVector();

public:
	bool empty();

	std::string getBlob(uint32_t columnIndex) const;
	std::string getBlob(const std::string& columnLabel) const;

	bool getBoolean(uint32_t columnIndex) const;
	bool getBoolean(const std::string& columnLabel) const;

	double getDouble(uint32_t columnIndex) const;
	double getDouble(const std::string& columnLabel) const;

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

	int32_t rowsAffected();

	int32_t rowsCount();
	int32_t columnCount();

protected:
	bool check() const;

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<QSqlQuery> m_spSqlQuery;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};