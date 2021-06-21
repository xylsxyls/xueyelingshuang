#pragma once
#include "SQLiteMacro.h"
#include <stdint.h>
#include <string>
#include <memory>

struct SqliteDatabase;

/** sql语句管理类
*/
class SQLiteAPI SQLitePrepareStatement
{
	friend class SQLiteResultSet;
public:
	/** 构造函数
	@param [in] db 语句指针
	*/
	SQLitePrepareStatement(const std::shared_ptr<SqliteDatabase>& spDb);

public:
	bool empty();

	bool prepare(const std::string& sqlString);

	void setBlob(uint32_t index, const std::string& value);

	void setBoolean(uint32_t index, bool value);

	void setInt(uint32_t index, int32_t value);

	void setDouble(uint32_t index, double value);

	//遇到\0就停止存储了，存储内容不带\0
	void setString(uint32_t index, const std::string& value);

	void setUnsignedInt(uint32_t index, uint32_t value);

	void setLongLong(uint32_t index, int64_t value);

	void setUnsignedLongLong(uint32_t index, uint64_t value);

	bool exec();

	//当需要重新设置替换字符内容时需要先执行reset，reset会把执行结果清空，但不会清空prepare的字符串
	void reset();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<SqliteDatabase> m_spDb;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};