#include "QSQLiteResultSet.h"
#include <QSqlQuery>
#include <QVariant>
#include "QSQLitePrepareStatement.h"

QSQLiteResultSet::QSQLiteResultSet() :
m_spSqlQuery(nullptr)
{

}

QSQLiteResultSet::QSQLiteResultSet(QSQLitePrepareStatement& prepareStatement)
{
	if (!prepareStatement.empty())
	{
		m_spSqlQuery = prepareStatement.m_spSqlQuery;
	}
}

std::string QSQLiteResultSet::getBlob(uint32_t columnIndex) const
{
	if (!check())
	{
		return "";
	}
	try
	{
		return m_spSqlQuery->value(columnIndex).toByteArray().toStdString();
	}
	catch (...)
	{
		return "";
	}
}

std::string QSQLiteResultSet::getBlob(const std::string& columnLabel) const
{
	if (!check())
	{
		return "";
	}
	try
	{
		return m_spSqlQuery->value(columnLabel.c_str()).toByteArray().toStdString();
	}
	catch (...)
	{
		return "";
	}
}

bool QSQLiteResultSet::getBoolean(uint32_t columnIndex) const
{
	if (!check())
	{
		return false;
	}
	try
	{
		return m_spSqlQuery->value(columnIndex).toBool();
	}
	catch (...)
	{
		return false;
	}
}

bool QSQLiteResultSet::getBoolean(const std::string& columnLabel) const
{
	if (!check())
	{
		return false;
	}
	try
	{
		return m_spSqlQuery->value(columnLabel.c_str()).toBool();
	}
	catch (...)
	{
		return false;
	}
}

double QSQLiteResultSet::getDouble(uint32_t columnIndex) const
{
	if (!check())
	{
		return 0;
	}
	try
	{
		return m_spSqlQuery->value(columnIndex).toDouble();
	}
	catch (...)
	{
		return 0;
	}
}

double QSQLiteResultSet::getDouble(const std::string& columnLabel) const
{
	if (!check())
	{
		return 0;
	}
	try
	{
		return m_spSqlQuery->value(columnLabel.c_str()).toDouble();
	}
	catch (...)
	{
		return 0;
	}
}

int32_t QSQLiteResultSet::getInt(uint32_t columnIndex) const
{
	if (!check())
	{
		return 0;
	}
	try
	{
		return m_spSqlQuery->value(columnIndex).toInt();
	}
	catch (...)
	{
		return 0;
	}
}

int32_t QSQLiteResultSet::getInt(const std::string& columnLabel) const
{
	if (!check())
	{
		return 0;
	}
	try
	{
		return m_spSqlQuery->value(columnLabel.c_str()).toInt();
	}
	catch (...)
	{
		return 0;
	}
}

uint32_t QSQLiteResultSet::getUInt(uint32_t columnIndex) const
{
	if (!check())
	{
		return 0;
	}
	try
	{
		return m_spSqlQuery->value(columnIndex).toUInt();
	}
	catch (...)
	{
		return 0;
	}
}

uint32_t QSQLiteResultSet::getUInt(const std::string& columnLabel) const
{
	if (!check())
	{
		return 0;
	}
	try
	{
		return m_spSqlQuery->value(columnLabel.c_str()).toString().toUInt();
	}
	catch (...)
	{
		return 0;
	}
}

int64_t QSQLiteResultSet::getInt64(uint32_t columnIndex) const
{
	if (!check())
	{
		return 0;
	}
	try
	{
		return m_spSqlQuery->value(columnIndex).toLongLong();
	}
	catch (...)
	{
		return 0;
	}
}

int64_t QSQLiteResultSet::getInt64(const std::string& columnLabel) const
{
	if (!check())
	{
		return 0;
	}
	try
	{
		return m_spSqlQuery->value(columnLabel.c_str()).toString().toLongLong();
	}
	catch (...)
	{
		return 0;
	}
}

uint64_t QSQLiteResultSet::getUInt64(uint32_t columnIndex) const
{
	if (!check())
	{
		return 0;
	}
	try
	{
		return m_spSqlQuery->value(columnIndex).toULongLong();
	}
	catch (...)
	{
		return 0;
	}
}

uint64_t QSQLiteResultSet::getUInt64(const std::string& columnLabel) const
{
	if (!check())
	{
		return 0;
	}
	try
	{
		return m_spSqlQuery->value(columnLabel.c_str()).toString().toULongLong();
	}
	catch (...)
	{
		return 0;
	}
}

std::string QSQLiteResultSet::getString(uint32_t columnIndex) const
{
	if (!check())
	{
		return "";
	}
	try
	{
		return m_spSqlQuery->value(columnIndex).toString().toStdString();
	}
	catch (...)
	{
		return "";
	}
}

std::string QSQLiteResultSet::getString(const std::string& columnLabel) const
{
	if (!check())
	{
		return "";
	}
	try
	{
		return m_spSqlQuery->value(columnLabel.c_str()).toString().toStdString();
	}
	catch (...)
	{
		return "";
	}
}

bool QSQLiteResultSet::next()
{
	if (!check())
	{
		return false;
	}
	try
	{
		return m_spSqlQuery->next();
	}
	catch (...)
	{
		return false;
	}
}

bool QSQLiteResultSet::previous()
{
	if (!check())
	{
		return false;
	}
	try
	{
		return m_spSqlQuery->previous();
	}
	catch (...)
	{
		return false;
	}
}

int32_t QSQLiteResultSet::rowsAffected()
{
	if (!check())
	{
		return 0;
	}
	try
	{
		return m_spSqlQuery->numRowsAffected();
	}
	catch (...)
	{
		return 0;
	}
}

int32_t QSQLiteResultSet::rowsCount()
{
	int32_t rows = 0;
	while (next())
	{
		++rows;
	}
	int32_t count = rows + 1;
	while (count-- != 0)
	{
		previous();
	}
	return rows;
}

std::vector<std::vector<std::string>> QSQLiteResultSet::toVector()
{
	std::vector<std::vector<std::string>> result;
	int32_t column = columnCount();
	while (next())
	{
		std::vector<std::string> vecRow;
		int32_t index = -1;
		while (index++ != column - 1)
		{
			vecRow.push_back(getString(index));
		}
		result.push_back(vecRow);
	}
	return result;
}

bool QSQLiteResultSet::empty()
{
	return m_spSqlQuery != nullptr;
}

int32_t QSQLiteResultSet::columnCount()
{
	if (!check())
	{
		return 0;
	}
	int32_t columnCount = -1;
	bool hasNext = next();
	if (hasNext == false)
	{
		previous();
		return 0;
	}
	bool btrue = true;
	while (btrue)
	{
		try
		{
			if (!m_spSqlQuery->value(++columnCount).isValid())
			{
				break;
			}
		}
		catch (...)
		{
			break;
		}
	}
	previous();
	return columnCount;
}

bool QSQLiteResultSet::check() const
{
	return m_spSqlQuery != nullptr;
}