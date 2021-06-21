#include "SQLitePrepareStatement.h"
#include "sqlite3.h"
#include "SqliteDatabase.h"

SQLitePrepareStatement::SQLitePrepareStatement(const std::shared_ptr<SqliteDatabase>& spDb) :
m_spDb(spDb)
{

}

bool SQLitePrepareStatement::empty()
{
	return (m_spDb == nullptr);
}

bool SQLitePrepareStatement::prepare(const std::string& sqlString)
{
	if (m_spDb == nullptr)
	{
		return false;
	}
	try
	{
		if (m_spDb->m_stmt != nullptr)
		{
			int32_t result = sqlite3_finalize(m_spDb->m_stmt);
			if (result != 0)
			{
				printf("prepare sqlite3_finalize failed, result = %d\n", result);
			}
			m_spDb->m_stmt = nullptr;
		}
		int32_t result = sqlite3_prepare(m_spDb->m_db, sqlString.c_str(), sqlString.size(), &m_spDb->m_stmt, nullptr);
		if (result != 0)
		{
			printf("prepare sqlite3_prepare failed, result = %d\n", result);
			m_spDb->m_stmt = nullptr;
		}
		return result == 0;
	}
	catch (...)
	{
		return false;
	}
}

void SQLitePrepareStatement::setBlob(uint32_t index, const std::string& value)
{
	if (value.empty() || m_spDb == nullptr)
	{
		return;
	}
	try
	{
		int32_t result = sqlite3_bind_blob(m_spDb->m_stmt, index + 1, value.c_str(), value.size(), SQLITE_TRANSIENT);
		if (result != 0)
		{
			printf("setBlob failed, result = %d\n", result);
		}
	}
	catch (...)
	{
		
	}
}

void SQLitePrepareStatement::setBoolean(uint32_t index, bool value)
{
	if (m_spDb == nullptr || m_spDb == nullptr)
	{
		return;
	}
	try
	{
		int32_t result = sqlite3_bind_int(m_spDb->m_stmt, index + 1, (int32_t)value);
		if (result != 0)
		{
			printf("setBoolean failed, result = %d\n", result);
		}
	}
	catch (...)
	{

	}
}

void SQLitePrepareStatement::setInt(uint32_t index, int32_t value)
{
	if (m_spDb == nullptr)
	{
		return;
	}
	try
	{
		int32_t result = sqlite3_bind_int(m_spDb->m_stmt, index + 1, value);
		if (result != 0)
		{
			printf("setInt failed, result = %d\n", result);
		}
	}
	catch (...)
	{

	}
}

void SQLitePrepareStatement::setDouble(uint32_t index, double value)
{
	if (m_spDb == nullptr)
	{
		return;
	}
	try
	{
		int32_t result = sqlite3_bind_double(m_spDb->m_stmt, index + 1, value);
		if (result != 0)
		{
			printf("setDouble failed, result = %d\n", result);
		}
	}
	catch (...)
	{

	}
}

void SQLitePrepareStatement::setString(uint32_t index, const std::string& value)
{
	if (m_spDb == nullptr)
	{
		return;
	}
	try
	{
		int32_t result = sqlite3_bind_text(m_spDb->m_stmt, index + 1, value.c_str(), -1, SQLITE_TRANSIENT);
		if (result != 0)
		{
			printf("setString failed, result = %d\n", result);
		}
	}
	catch (...)
	{

	}
}

void SQLitePrepareStatement::setUnsignedInt(uint32_t index, uint32_t value)
{
	if (m_spDb == nullptr)
	{
		return;
	}
	try
	{
		int32_t result = sqlite3_bind_int(m_spDb->m_stmt, index + 1, (int32_t)value);
		if (result != 0)
		{
			printf("setUnsignedInt failed, result = %d\n", result);
		}
	}
	catch (...)
	{

	}
}

void SQLitePrepareStatement::setLongLong(uint32_t index, int64_t value)
{
	if (m_spDb == nullptr)
	{
		return;
	}
	try
	{
		int32_t result = sqlite3_bind_int64(m_spDb->m_stmt, index + 1, value);
		if (result != 0)
		{
			printf("setLongLong failed, result = %d\n", result);
		}
	}
	catch (...)
	{

	}
}

void SQLitePrepareStatement::setUnsignedLongLong(uint32_t index, uint64_t value)
{
	if (m_spDb == nullptr)
	{
		return;
	}
	try
	{
		int32_t result = sqlite3_bind_int64(m_spDb->m_stmt, index + 1, (int64_t)value);
		if (result != 0)
		{
			printf("setUnsignedLongLong failed, result = %d\n", result);
		}
	}
	catch (...)
	{

	}
}

bool SQLitePrepareStatement::exec()
{
	if (m_spDb == nullptr)
	{
		return false;
	}
	try
	{
		m_spDb->m_stepResult = sqlite3_step(m_spDb->m_stmt);

		if (m_spDb->m_stepResult != SQLITE_DONE && m_spDb->m_stepResult != SQLITE_ROW)
		{
			printf("sqlite3_step failed, result = %d\n", m_spDb->m_stepResult);
			return false;
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
}

void SQLitePrepareStatement::reset()
{
	if (m_spDb == nullptr)
	{
		return;
	}
	m_spDb->m_stepResult = 0;
	m_spDb->m_isBeginRead = false;
	int32_t result = sqlite3_reset(m_spDb->m_stmt);
	if (result != 0)
	{
		printf("reset failed, result = %d\n", result);
	}
}