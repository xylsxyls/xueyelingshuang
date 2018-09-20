#include "SQLiteResultSet.h"
#include <QSqlQuery>
#include <QVariant>
#include "SQLitePrepareStatement.h"

SQLiteResultSet::SQLiteResultSet(const std::shared_ptr<QSqlQuery>& spSqlQuery) :
m_spSqlQuery(spSqlQuery)
{

}

std::string SQLiteResultSet::getBlob(uint32_t columnIndex) const
{
	try
	{
		return m_spSqlQuery->value(columnIndex).toByteArray().toStdString();
	}
	catch (...)
	{
		return "";
	}
}

std::string SQLiteResultSet::getBlob(const std::string& columnLabel) const
{
	try
	{
		return m_spSqlQuery->value(columnLabel.c_str()).toByteArray().toStdString();
	}
	catch (...)
	{
		return "";
	}
}

bool SQLiteResultSet::getBoolean(uint32_t columnIndex) const
{
	try
	{
		return m_spSqlQuery->value(columnIndex).toBool();
	}
	catch (...)
	{
		return false;
	}
}

bool SQLiteResultSet::getBoolean(const std::string& columnLabel) const
{
	try
	{
		return m_spSqlQuery->value(columnLabel.c_str()).toBool();
	}
	catch (...)
	{
		return false;
	}
}

double SQLiteResultSet::getDouble(uint32_t columnIndex) const
{
	try
	{
		return m_spSqlQuery->value(columnIndex).toDouble();
	}
	catch (...)
	{
		return 0;
	}
}

double SQLiteResultSet::getDouble(const std::string& columnLabel) const
{
	try
	{
		return m_spSqlQuery->value(columnLabel.c_str()).toDouble();
	}
	catch (...)
	{
		return 0;
	}
}

int32_t SQLiteResultSet::getInt(uint32_t columnIndex) const
{
	try
	{
		return m_spSqlQuery->value(columnIndex).toInt();
	}
	catch (...)
	{
		return 0;
	}
}

int32_t SQLiteResultSet::getInt(const std::string& columnLabel) const
{
	try
	{
		return m_spSqlQuery->value(columnLabel.c_str()).toInt();
	}
	catch (...)
	{
		return 0;
	}
}

uint32_t SQLiteResultSet::getUInt(uint32_t columnIndex) const
{
	try
	{
		return m_spSqlQuery->value(columnIndex).toUInt();
	}
	catch (...)
	{
		return 0;
	}
}

uint32_t SQLiteResultSet::getUInt(const std::string& columnLabel) const
{
	try
	{
		return m_spSqlQuery->value(columnLabel.c_str()).toString().toUInt();
	}
	catch (...)
	{
		return 0;
	}
}

int64_t SQLiteResultSet::getInt64(uint32_t columnIndex) const
{
	try
	{
		return m_spSqlQuery->value(columnIndex).toLongLong();
	}
	catch (...)
	{
		return 0;
	}
}

int64_t SQLiteResultSet::getInt64(const std::string& columnLabel) const
{
	try
	{
		return m_spSqlQuery->value(columnLabel.c_str()).toString().toLongLong();
	}
	catch (...)
	{
		return 0;
	}
}

uint64_t SQLiteResultSet::getUInt64(uint32_t columnIndex) const
{
	try
	{
		return m_spSqlQuery->value(columnIndex).toULongLong();
	}
	catch (...)
	{
		return 0;
	}
}

uint64_t SQLiteResultSet::getUInt64(const std::string& columnLabel) const
{
	try
	{
		return m_spSqlQuery->value(columnLabel.c_str()).toString().toULongLong();
	}
	catch (...)
	{
		return 0;
	}
}

std::string SQLiteResultSet::getString(uint32_t columnIndex) const
{
	try
	{
		return m_spSqlQuery->value(columnIndex).toString().toStdString();
	}
	catch (...)
	{
		return "";
	}
}

std::string SQLiteResultSet::getString(const std::string& columnLabel) const
{
	try
	{
		return m_spSqlQuery->value(columnLabel.c_str()).toString().toStdString();
	}
	catch (...)
	{
		return "";
	}
}

bool SQLiteResultSet::next()
{
	try
	{
		return m_spSqlQuery->next();
	}
	catch (...)
	{
		return false;
	}
}

bool SQLiteResultSet::previous()
{
	try
	{
		return m_spSqlQuery->previous();
	}
	catch (...)
	{
		return false;
	}
}