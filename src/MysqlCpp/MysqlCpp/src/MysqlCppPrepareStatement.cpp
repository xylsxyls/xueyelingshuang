#include "MysqlCppPrepareStatement.h"
#include "MysqlCppExecution.h"
#include "MysqlCppHelper.h"
#include "MysqlCppResultSet.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#include "mysql-connector-cpp/cppconn/prepared_statement.h"
#include "mysql-connector-cpp/cppconn/exception.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

MysqlCppPrepareStatement::MysqlCppPrepareStatement(const MysqlCppExecution& execution, const std::string& sqlString) :
m_session(execution.m_session),
m_preparedStatement(nullptr),
m_sqlString(sqlString)
{
    if (!execution.m_connection)
    {
        m_error = "mysql connection is not available";
        return;
    }
    try
    {
        m_preparedStatement = execution.m_connection->prepareStatement(sqlString);
        if (!m_preparedStatement)
        {
            m_error = "prepare statement returned null";
        }
    }
    catch (const sql::SQLException& error)
    {
        m_error = MysqlCppHelper::exceptionText(error);
    }
    catch (...)
    {
        m_error = "prepare statement failed";
    }
}

bool MysqlCppPrepareStatement::selectForExecution(MysqlCppExecution& execution)
{
    execution.m_statement = nullptr;
    execution.m_cursor = nullptr;
    if (!isValid() || m_session.owner_before(execution.m_session) || execution.m_session.owner_before(m_session))
    {
        return false;
    }
    execution.m_statement = m_preparedStatement;
    execution.m_cursor = &m_cursor;
    return true;
}

MysqlCppPrepareStatement::~MysqlCppPrepareStatement()
{
    delete m_preparedStatement;
}

void MysqlCppPrepareStatement::close()
{
    sql::PreparedStatement* native = m_preparedStatement;
    m_preparedStatement = nullptr;
    auto cursor = m_cursor.lock();
    m_cursor.reset();
    if (cursor)
    {
        cursor->close();
    }
    delete native;
}

bool MysqlCppPrepareStatement::isValid() const
{
    return m_preparedStatement != nullptr && !m_session.expired();
}

std::string MysqlCppPrepareStatement::lastError() const
{
    return m_error;
}

std::string MysqlCppPrepareStatement::sql() const
{
    return m_sqlString;
}

void MysqlCppPrepareStatement::setBigInt(uint32_t parameterIndex, const std::string& value)
{
    bindParameter<MysqlCppHelper>(parameterIndex, [this, &value](uint32_t index)
    {
        m_preparedStatement->setBigInt(index, value);
    });
}

void MysqlCppPrepareStatement::setBlob(uint32_t parameterIndex, std::istream* blob)
{
    if (blob == nullptr && isValid())
    {
        m_session.reset();
        m_error = "null BLOB stream; use setNull for SQL NULL";
        return;
    }
    bindParameter<MysqlCppHelper>(parameterIndex, [this, blob](uint32_t index)
    {
        m_preparedStatement->setBlob(index, blob);
    });
}

void MysqlCppPrepareStatement::setBoolean(uint32_t parameterIndex, bool value)
{
    bindParameter<MysqlCppHelper>(parameterIndex, [this, value](uint32_t index)
    {
        m_preparedStatement->setBoolean(index, value);
    });
}

void MysqlCppPrepareStatement::setDateTime(uint32_t parameterIndex, const std::string& value)
{
    bindParameter<MysqlCppHelper>(parameterIndex, [this, &value](uint32_t index)
    {
        m_preparedStatement->setDateTime(index, value);
    });
}

void MysqlCppPrepareStatement::setDouble(uint32_t parameterIndex, double value)
{
    bindParameter<MysqlCppHelper>(parameterIndex, [this, value](uint32_t index)
    {
        m_preparedStatement->setDouble(index, value);
    });
}

void MysqlCppPrepareStatement::setInt(uint32_t parameterIndex, int32_t value)
{
    bindParameter<MysqlCppHelper>(parameterIndex, [this, value](uint32_t index)
    {
        m_preparedStatement->setInt(index, value);
    });
}

void MysqlCppPrepareStatement::setUInt(uint32_t parameterIndex, uint32_t value)
{
    bindParameter<MysqlCppHelper>(parameterIndex, [this, value](uint32_t index)
    {
        m_preparedStatement->setUInt(index, value);
    });
}

void MysqlCppPrepareStatement::setInt64(uint32_t parameterIndex, int64_t value)
{
    bindParameter<MysqlCppHelper>(parameterIndex, [this, value](uint32_t index)
    {
        m_preparedStatement->setInt64(index, value);
    });
}

void MysqlCppPrepareStatement::setUInt64(uint32_t parameterIndex, uint64_t value)
{
    bindParameter<MysqlCppHelper>(parameterIndex, [this, value](uint32_t index)
    {
        m_preparedStatement->setUInt64(index, value);
    });
}

void MysqlCppPrepareStatement::setNull(uint32_t parameterIndex, int32_t sqlType)
{
    bindParameter<MysqlCppHelper>(parameterIndex, [this, sqlType](uint32_t index)
    {
        m_preparedStatement->setNull(index, sqlType);
    });
}

void MysqlCppPrepareStatement::setString(uint32_t parameterIndex, const std::string& value)
{
    bindParameter<MysqlCppHelper>(parameterIndex, [this, &value](uint32_t index)
    {
        m_preparedStatement->setString(index, value);
    });
}