#include "SQLiteResultSet.h"
#include "SQLitePrepareStatement.h"
#include "sqlite3.h"
#include "SqliteDatabase.h"
#include "CStringManager/CStringManagerAPI.h"
#include <algorithm>

SQLiteResultSet::SQLiteResultSet() :
m_spDb(nullptr)
{

}

SQLiteResultSet::SQLiteResultSet(SQLitePrepareStatement& prepareStatement)
{
	if (!prepareStatement.empty())
	{
		m_vecTableNameBase.clear();
		m_vecTableName.clear();
		m_spDb = prepareStatement.m_spDb;
		//在经过预编译之后就能查到表头，不需要sqlite3_step
		int32_t count = sqlite3_column_count(m_spDb->m_stmt);
		int32_t index = -1;
		while (index++ != count - 1)
		{
			const char* tableName = sqlite3_column_name(m_spDb->m_stmt, index);
			m_vecTableNameBase.push_back(tableName);
			m_vecTableName.push_back(CStringManager::MakeUpper(tableName));
		}
	}
}

std::string SQLiteResultSet::getBlob(uint32_t columnIndex) const
{
	if (!check() || m_spDb->m_stepResult != SQLITE_ROW)
	{
		return "";
	}
	try
	{
		const void* data = sqlite3_column_blob(m_spDb->m_stmt, columnIndex);
		int32_t length = sqlite3_column_bytes(m_spDb->m_stmt, columnIndex);
		return std::string((char*)data, length);
	}
	catch (...)
	{
		return "";
	}
}

std::string SQLiteResultSet::getBlob(const std::string& columnLabel) const
{
	auto itTableName = std::find(m_vecTableName.begin(), m_vecTableName.end(), CStringManager::MakeUpper(columnLabel));
	int32_t columnIndex = (int32_t)std::distance(m_vecTableName.begin(), itTableName);
	if (columnIndex == m_vecTableName.size())
	{
		return "";
	}
	return getBlob(columnIndex);
}

bool SQLiteResultSet::getBoolean(uint32_t columnIndex) const
{
	if (!check() || m_spDb->m_stepResult != SQLITE_ROW)
	{
		return false;
	}
	try
	{
		return sqlite3_column_int(m_spDb->m_stmt, columnIndex) == 1;
	}
	catch (...)
	{
		return false;
	}
}

bool SQLiteResultSet::getBoolean(const std::string& columnLabel) const
{
	auto itTableName = std::find(m_vecTableName.begin(), m_vecTableName.end(), CStringManager::MakeUpper(columnLabel));
	int32_t columnIndex = (int32_t)std::distance(m_vecTableName.begin(), itTableName);
	if (columnIndex == m_vecTableName.size())
	{
		return false;
	}
	return getBoolean(columnIndex);
}

double SQLiteResultSet::getDouble(uint32_t columnIndex) const
{
	if (!check() || m_spDb->m_stepResult != SQLITE_ROW)
	{
		return 0;
	}
	try
	{
		return sqlite3_column_double(m_spDb->m_stmt, columnIndex);
	}
	catch (...)
	{
		return 0;
	}
}

double SQLiteResultSet::getDouble(const std::string& columnLabel) const
{
	auto itTableName = std::find(m_vecTableName.begin(), m_vecTableName.end(), CStringManager::MakeUpper(columnLabel));
	int32_t columnIndex = (int32_t)std::distance(m_vecTableName.begin(), itTableName);
	if (columnIndex == m_vecTableName.size())
	{
		return 0;
	}
	return getDouble(columnIndex);
}

int32_t SQLiteResultSet::getInt(uint32_t columnIndex) const
{
	if (!check() || m_spDb->m_stepResult != SQLITE_ROW)
	{
		return 0;
	}
	try
	{
		return sqlite3_column_int(m_spDb->m_stmt, columnIndex);
	}
	catch (...)
	{
		return 0;
	}
}

int32_t SQLiteResultSet::getInt(const std::string& columnLabel) const
{
	auto itTableName = std::find(m_vecTableName.begin(), m_vecTableName.end(), CStringManager::MakeUpper(columnLabel));
	int32_t columnIndex = (int32_t)std::distance(m_vecTableName.begin(), itTableName);
	if (columnIndex == m_vecTableName.size())
	{
		return 0;
	}
	return getInt(columnIndex);
}

uint32_t SQLiteResultSet::getUInt(uint32_t columnIndex) const
{
	if (!check() || m_spDb->m_stepResult != SQLITE_ROW)
	{
		return 0;
	}
	try
	{
		return (uint32_t)sqlite3_column_int(m_spDb->m_stmt, columnIndex);
	}
	catch (...)
	{
		return 0;
	}
}

uint32_t SQLiteResultSet::getUInt(const std::string& columnLabel) const
{
	auto itTableName = std::find(m_vecTableName.begin(), m_vecTableName.end(), CStringManager::MakeUpper(columnLabel));
	int32_t columnIndex = (int32_t)std::distance(m_vecTableName.begin(), itTableName);
	if (columnIndex == m_vecTableName.size())
	{
		return 0;
	}
	return getUInt(columnIndex);
}

int64_t SQLiteResultSet::getInt64(uint32_t columnIndex) const
{
	if (!check() || m_spDb->m_stepResult != SQLITE_ROW)
	{
		return 0;
	}
	try
	{
		return sqlite3_column_int64(m_spDb->m_stmt, columnIndex);
	}
	catch (...)
	{
		return 0;
	}
}

int64_t SQLiteResultSet::getInt64(const std::string& columnLabel) const
{
	auto itTableName = std::find(m_vecTableName.begin(), m_vecTableName.end(), CStringManager::MakeUpper(columnLabel));
	int32_t columnIndex = (int32_t)std::distance(m_vecTableName.begin(), itTableName);
	if (columnIndex == m_vecTableName.size())
	{
		return 0;
	}
	return getInt64(columnIndex);
}

uint64_t SQLiteResultSet::getUInt64(uint32_t columnIndex) const
{
	if (!check() || m_spDb->m_stepResult != SQLITE_ROW)
	{
		return 0;
	}
	try
	{
		return (uint64_t)sqlite3_column_int64(m_spDb->m_stmt, columnIndex);
	}
	catch (...)
	{
		return 0;
	}
}

uint64_t SQLiteResultSet::getUInt64(const std::string& columnLabel) const
{
	auto itTableName = std::find(m_vecTableName.begin(), m_vecTableName.end(), CStringManager::MakeUpper(columnLabel));
	int32_t columnIndex = (int32_t)std::distance(m_vecTableName.begin(), itTableName);
	if (columnIndex == m_vecTableName.size())
	{
		return 0;
	}
	return getUInt64(columnIndex);
}

std::string SQLiteResultSet::getString(uint32_t columnIndex) const
{
	if (!check() || m_spDb->m_stepResult != SQLITE_ROW)
	{
		return "";
	}
	try
	{
		const unsigned char* data = sqlite3_column_text(m_spDb->m_stmt, columnIndex);
		int32_t length = sqlite3_column_bytes(m_spDb->m_stmt, columnIndex);
		return std::string((char*)data, length);
	}
	catch (...)
	{
		return "";
	}
}

std::string SQLiteResultSet::getString(const std::string& columnLabel) const
{
	auto itTableName = std::find(m_vecTableName.begin(), m_vecTableName.end(), CStringManager::MakeUpper(columnLabel));
	int32_t columnIndex = (int32_t)std::distance(m_vecTableName.begin(), itTableName);
	if (columnIndex == m_vecTableName.size())
	{
		return "";
	}
	return getString(columnIndex);
}

bool SQLiteResultSet::next()
{
	if (!check() || m_spDb->m_stepResult != SQLITE_ROW)
	{
		return false;
	}
	try
	{
		if (m_spDb->m_stepResult == SQLITE_ROW && !m_spDb->m_isBeginRead)
		{
			m_spDb->m_isBeginRead = true;
			return true;
		}
		m_spDb->m_stepResult = sqlite3_step(m_spDb->m_stmt);
		if (m_spDb->m_stepResult != SQLITE_DONE && m_spDb->m_stepResult != SQLITE_ROW)
		{
			printf("next failed, stepResult = %d\n", m_spDb->m_stepResult);
			return false;
		}
		if (m_spDb->m_stepResult == SQLITE_ROW)
		{
			return true;
		}
		return false;
	}
	catch (...)
	{
		return false;
	}
}

std::vector<std::string> SQLiteResultSet::tableName()
{
	return m_vecTableNameBase;
}

int32_t SQLiteResultSet::rowsAffected()
{
	if (!check())
	{
		return 0;
	}
	try
	{
		return sqlite3_changes(m_spDb->m_db);
	}
	catch (...)
	{
		return 0;
	}
}

int32_t SQLiteResultSet::rowsCount()
{
	int32_t rows = 0;
	while (next())
	{
		++rows;
	}
	int32_t result = sqlite3_reset(m_spDb->m_stmt);
	if (result != 0)
	{
		printf("rowsCount reset failed, result = %d\n", result);
		return 0;
	}
	m_spDb->m_isBeginRead = false;
	m_spDb->m_stepResult = sqlite3_step(m_spDb->m_stmt);
	if (m_spDb->m_stepResult != SQLITE_ROW && m_spDb->m_stepResult != SQLITE_DONE)
	{
		printf("rowsCount sqlite3_step failed, result = %d\n", m_spDb->m_stepResult);
		return 0;
	}
	return rows;
}

std::vector<std::vector<std::string>> SQLiteResultSet::toVector()
{
	std::vector<std::vector<std::string>> result;
	int32_t column = columnCount();
	while (next())
	{
		std::vector<std::string> vecRow;
		int32_t index = -1;
		while (index++ != column - 1)
		{
			vecRow.push_back(getBlob(index));
		}
		result.push_back(vecRow);
	}
	return result;
}

bool SQLiteResultSet::empty()
{
	return (m_spDb == nullptr);
}

int32_t SQLiteResultSet::columnCount()
{
	if (!check())
	{
		return 0;
	}
	return sqlite3_column_count(m_spDb->m_stmt);
}

bool SQLiteResultSet::check() const
{
	return (m_spDb != nullptr);
}