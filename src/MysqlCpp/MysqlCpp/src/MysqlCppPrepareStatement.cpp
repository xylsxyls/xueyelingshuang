#include "MysqlCppPrepareStatement.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#include "cppconn/prepared_statement.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

MysqlCppPrepareStatement::MysqlCppPrepareStatement(sql::PreparedStatement* prepareStatement) :
m_preparedStatement(prepareStatement)
{
	
}

MysqlCppPrepareStatement::~MysqlCppPrepareStatement()
{
	delete m_preparedStatement;
}

void MysqlCppPrepareStatement::setBigInt(unsigned int parameterIndex, const std::string& value)
{
	if (m_preparedStatement == nullptr)
	{
		return;
	}
	try
	{
		m_preparedStatement->setBigInt(parameterIndex, value);
	}
	catch (...)
	{
		return;
	}
}

void MysqlCppPrepareStatement::setBlob(unsigned int parameterIndex, std::istream * blob)
{
	if (m_preparedStatement == nullptr)
	{
		return;
	}
	try
	{
		m_preparedStatement->setBlob(parameterIndex, blob);
	}
	catch (...)
	{
		return;
	}
}

void MysqlCppPrepareStatement::setBoolean(unsigned int parameterIndex, bool value)
{
	if (m_preparedStatement == nullptr)
	{
		return;
	}
	try
	{
		m_preparedStatement->setBoolean(parameterIndex, value);
	}
	catch (...)
	{
		return;
	}
}

void MysqlCppPrepareStatement::setDateTime(unsigned int parameterIndex, const std::string& value)
{
	if (m_preparedStatement == nullptr)
	{
		return;
	}
	try
	{
		m_preparedStatement->setDateTime(parameterIndex, value);
	}
	catch (...)
	{
		return;
	}
}

void MysqlCppPrepareStatement::setDouble(unsigned int parameterIndex, double value)
{
	if (m_preparedStatement == nullptr)
	{
		return;
	}
	try
	{
		m_preparedStatement->setDouble(parameterIndex, value);
	}
	catch (...)
	{
		return;
	}
}

void MysqlCppPrepareStatement::setInt(unsigned int parameterIndex, int32_t value)
{
	if (m_preparedStatement == nullptr)
	{
		return;
	}
	try
	{
		m_preparedStatement->setInt(parameterIndex, value);
	}
	catch (...)
	{
		return;
	}
}

void MysqlCppPrepareStatement::setUInt(unsigned int parameterIndex, uint32_t value)
{
	if (m_preparedStatement == nullptr)
	{
		return;
	}
	try
	{
		m_preparedStatement->setUInt(parameterIndex, value);
	}
	catch (...)
	{
		return;
	}
}

void MysqlCppPrepareStatement::setInt64(unsigned int parameterIndex, int64_t value)
{
	if (m_preparedStatement == nullptr)
	{
		return;
	}
	try
	{
		m_preparedStatement->setInt64(parameterIndex, value);
	}
	catch (...)
	{
		return;
	}
}

void MysqlCppPrepareStatement::setUInt64(unsigned int parameterIndex, uint64_t value)
{
	if (m_preparedStatement == nullptr)
	{
		return;
	}
	try
	{
		m_preparedStatement->setUInt64(parameterIndex, value);
	}
	catch (...)
	{
		return;
	}
}

void MysqlCppPrepareStatement::setNull(unsigned int parameterIndex, int sqlType)
{
	if (m_preparedStatement == nullptr)
	{
		return;
	}
	try
	{
		m_preparedStatement->setNull(parameterIndex, sqlType);
	}
	catch (...)
	{
		return;
	}
}

void MysqlCppPrepareStatement::setString(unsigned int parameterIndex, const std::string& value)
{
	if (m_preparedStatement == nullptr)
	{
		return;
	}
	try
	{
		m_preparedStatement->setString(parameterIndex, value);
	}
	catch (...)
	{
		return;
	}
}

sql::PreparedStatement* MysqlCppPrepareStatement::preparedStatement() const
{
	return m_preparedStatement;
}
