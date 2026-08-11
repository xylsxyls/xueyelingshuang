#include "MysqlCppPrepareStatement.h"
#include "MysqlCppHelper.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#include "cppconn/prepared_statement.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

MysqlCppPrepareStatement::MysqlCppPrepareStatement(sql::PreparedStatement* prepareStatement, const std::string& sqlString) :
m_preparedStatement(prepareStatement),
m_sqlString(sqlString)
{

}

MysqlCppPrepareStatement::~MysqlCppPrepareStatement()
{
    delete m_preparedStatement;
}

bool MysqlCppPrepareStatement::isValid() const
{
    return m_preparedStatement != nullptr;
}

std::string MysqlCppPrepareStatement::sql() const
{
    return m_sqlString;
}

void MysqlCppPrepareStatement::setBigInt(uint32_t parameterIndex, const std::string& value)
{
    if (m_preparedStatement == nullptr)
    {
        return;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(parameterIndex);
    if (driverIndex == 0)
    {
        return;
    }
    try
    {
        m_preparedStatement->setBigInt(driverIndex, value);
    }
    catch (...)
    {
        return;
    }
}

void MysqlCppPrepareStatement::setBlob(uint32_t parameterIndex, std::istream* blob)
{
    if (m_preparedStatement == nullptr)
    {
        return;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(parameterIndex);
    if (driverIndex == 0)
    {
        return;
    }
    try
    {
        m_preparedStatement->setBlob(driverIndex, blob);
    }
    catch (...)
    {
        return;
    }
}

void MysqlCppPrepareStatement::setBoolean(uint32_t parameterIndex, bool value)
{
    if (m_preparedStatement == nullptr)
    {
        return;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(parameterIndex);
    if (driverIndex == 0)
    {
        return;
    }
    try
    {
        m_preparedStatement->setBoolean(driverIndex, value);
    }
    catch (...)
    {
        return;
    }
}

void MysqlCppPrepareStatement::setDateTime(uint32_t parameterIndex, const std::string& value)
{
    if (m_preparedStatement == nullptr)
    {
        return;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(parameterIndex);
    if (driverIndex == 0)
    {
        return;
    }
    try
    {
        m_preparedStatement->setDateTime(driverIndex, value);
    }
    catch (...)
    {
        return;
    }
}

void MysqlCppPrepareStatement::setDouble(uint32_t parameterIndex, double value)
{
    if (m_preparedStatement == nullptr)
    {
        return;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(parameterIndex);
    if (driverIndex == 0)
    {
        return;
    }
    try
    {
        m_preparedStatement->setDouble(driverIndex, value);
    }
    catch (...)
    {
        return;
    }
}

void MysqlCppPrepareStatement::setInt(uint32_t parameterIndex, int32_t value)
{
    if (m_preparedStatement == nullptr)
    {
        return;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(parameterIndex);
    if (driverIndex == 0)
    {
        return;
    }
    try
    {
        m_preparedStatement->setInt(driverIndex, value);
    }
    catch (...)
    {
        return;
    }
}

void MysqlCppPrepareStatement::setUInt(uint32_t parameterIndex, uint32_t value)
{
    if (m_preparedStatement == nullptr)
    {
        return;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(parameterIndex);
    if (driverIndex == 0)
    {
        return;
    }
    try
    {
        m_preparedStatement->setUInt(driverIndex, value);
    }
    catch (...)
    {
        return;
    }
}

void MysqlCppPrepareStatement::setInt64(uint32_t parameterIndex, int64_t value)
{
    if (m_preparedStatement == nullptr)
    {
        return;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(parameterIndex);
    if (driverIndex == 0)
    {
        return;
    }
    try
    {
        m_preparedStatement->setInt64(driverIndex, value);
    }
    catch (...)
    {
        return;
    }
}

void MysqlCppPrepareStatement::setUInt64(uint32_t parameterIndex, uint64_t value)
{
    if (m_preparedStatement == nullptr)
    {
        return;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(parameterIndex);
    if (driverIndex == 0)
    {
        return;
    }
    try
    {
        m_preparedStatement->setUInt64(driverIndex, value);
    }
    catch (...)
    {
        return;
    }
}

void MysqlCppPrepareStatement::setNull(uint32_t parameterIndex, int32_t sqlType)
{
    if (m_preparedStatement == nullptr)
    {
        return;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(parameterIndex);
    if (driverIndex == 0)
    {
        return;
    }
    try
    {
        m_preparedStatement->setNull(driverIndex, sqlType);
    }
    catch (...)
    {
        return;
    }
}

void MysqlCppPrepareStatement::setString(uint32_t parameterIndex, const std::string& value)
{
    if (m_preparedStatement == nullptr)
    {
        return;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(parameterIndex);
    if (driverIndex == 0)
    {
        return;
    }
    try
    {
        m_preparedStatement->setString(driverIndex, value);
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