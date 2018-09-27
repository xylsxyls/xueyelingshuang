#include "SQLitePrepareStatement.h"
#include <QSqlQuery>
#include <QVariant>

SQLitePrepareStatement::SQLitePrepareStatement(const std::shared_ptr<QSqlQuery>& spSqlQuery):
m_spSqlQuery(spSqlQuery)
{

}

void SQLitePrepareStatement::setBlob(uint32_t pos, const std::string& value)
{
	if (value.empty())
	{
		return;
	}
	try
	{
		m_spSqlQuery->bindValue(pos, QByteArray(&value[0], (int32_t)value.size()));
	}
	catch (...)
	{
		
	}
}

void SQLitePrepareStatement::setBoolean(uint32_t pos, bool value)
{
	try
	{
		m_spSqlQuery->bindValue(pos, value);
	}
	catch (...)
	{

	}
}

void SQLitePrepareStatement::setInt(uint32_t pos, int32_t value)
{
	try
	{
		m_spSqlQuery->bindValue(pos, value);
	}
	catch (...)
	{

	}
}

void SQLitePrepareStatement::setDouble(uint32_t pos, double value)
{
	try
	{
		m_spSqlQuery->bindValue(pos, value);
	}
	catch (...)
	{

	}
}

void SQLitePrepareStatement::setString(uint32_t pos, const std::string& value)
{
	try
	{
		m_spSqlQuery->bindValue(pos, value.c_str());
	}
	catch (...)
	{

	}
}

void SQLitePrepareStatement::setUnsignedInt(uint32_t pos, uint32_t value)
{
	try
	{
		m_spSqlQuery->bindValue(pos, value);
	}
	catch (...)
	{

	}
}

void SQLitePrepareStatement::setLongLong(uint32_t pos, int64_t value)
{
	try
	{
		m_spSqlQuery->bindValue(pos, value);
	}
	catch (...)
	{

	}
}

void SQLitePrepareStatement::setUnsignedLongLong(uint32_t pos, uint64_t value)
{
	try
	{
		m_spSqlQuery->bindValue(pos, value);
	}
	catch (...)
	{

	}
}

bool SQLitePrepareStatement::exec()
{
	try
	{
		return m_spSqlQuery->exec();
	}
	catch (...)
	{
		return false;
	}
}

std::shared_ptr<QSqlQuery> SQLitePrepareStatement::sqlQuery()
{
	return m_spSqlQuery;
}