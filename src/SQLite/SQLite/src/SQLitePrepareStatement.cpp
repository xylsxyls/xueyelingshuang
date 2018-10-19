#include "SQLitePrepareStatement.h"
#include <QSqlQuery>
#include <QVariant>
#include <QSqlDatabase>

SQLitePrepareStatement::SQLitePrepareStatement(QSqlDatabase* dataBase) :
m_spSqlQuery(nullptr)
{
	m_spSqlQuery.reset(new QSqlQuery(*dataBase));
}

bool SQLitePrepareStatement::empty()
{
	return m_spSqlQuery == nullptr;
}

bool SQLitePrepareStatement::prepare(const std::string& sqlString)
{
	if (m_spSqlQuery == nullptr)
	{
		return false;
	}
	try
	{
		return m_spSqlQuery->prepare(sqlString.c_str());
	}
	catch (...)
	{
		return false;
	}
}

void SQLitePrepareStatement::setBlob(uint32_t pos, const std::string& value)
{
	if (value.empty() || m_spSqlQuery == nullptr)
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
	if (m_spSqlQuery == nullptr)
	{
		return;
	}
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
	if (m_spSqlQuery == nullptr)
	{
		return;
	}
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
	if (m_spSqlQuery == nullptr)
	{
		return;
	}
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
	if (m_spSqlQuery == nullptr)
	{
		return;
	}
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
	if (m_spSqlQuery == nullptr)
	{
		return;
	}
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
	if (m_spSqlQuery == nullptr)
	{
		return;
	}
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
	if (m_spSqlQuery == nullptr)
	{
		return;
	}
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
	if (m_spSqlQuery == nullptr)
	{
		return false;
	}
	try
	{
		return m_spSqlQuery->exec();
	}
	catch (...)
	{
		return false;
	}
}