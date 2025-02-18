#pragma once
#include "QSQLiteMacro.h"
#include <stdint.h>
#include <string>
#include <memory>

class QSqlQuery;
class QVariant;
class QSqlDatabase;

/** sql语句管理类
*/
class QSQLiteAPI QSQLitePrepareStatement
{
	friend class QSQLiteResultSet;
public:
	/** 构造函数
	@param [in] prepareStatement 实际的操作指针
	*/
	QSQLitePrepareStatement(QSqlDatabase* dataBase);

public:
	bool empty();

	bool prepare(const std::string& sqlString);

	void setBlob(uint32_t pos, const std::string& value);

	void setBoolean(uint32_t pos, bool value);

	void setInt(uint32_t pos, int32_t value);

	void setDouble(uint32_t pos, double value);

	void setString(uint32_t pos, const std::string& value);

	void setUnsignedInt(uint32_t pos, uint32_t value);

	void setLongLong(uint32_t pos, int64_t value);

	void setUnsignedLongLong(uint32_t pos, uint64_t value);

	bool exec();

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