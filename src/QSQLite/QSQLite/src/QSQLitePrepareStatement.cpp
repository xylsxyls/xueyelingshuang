#include "QSQLitePrepareStatement.h"
#include <QSqlQuery>
#include <QVariant>
#include <QSqlDatabase>

QSQLitePrepareStatement::QSQLitePrepareStatement(QSqlDatabase* dataBase) :
m_spSqlQuery(nullptr)
{
	m_spSqlQuery.reset(new QSqlQuery(*dataBase));
}

bool QSQLitePrepareStatement::empty()
{
	return m_spSqlQuery == nullptr;
}

bool QSQLitePrepareStatement::prepare(const std::string& sqlString)
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

void QSQLitePrepareStatement::setBlob(uint32_t pos, const std::string& value)
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

void QSQLitePrepareStatement::setBoolean(uint32_t pos, bool value)
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

void QSQLitePrepareStatement::setInt(uint32_t pos, int32_t value)
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

void QSQLitePrepareStatement::setDouble(uint32_t pos, double value)
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

void QSQLitePrepareStatement::setString(uint32_t pos, const std::string& value)
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

void QSQLitePrepareStatement::setUnsignedInt(uint32_t pos, uint32_t value)
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

void QSQLitePrepareStatement::setLongLong(uint32_t pos, int64_t value)
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

void QSQLitePrepareStatement::setUnsignedLongLong(uint32_t pos, uint64_t value)
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

bool QSQLitePrepareStatement::exec()
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