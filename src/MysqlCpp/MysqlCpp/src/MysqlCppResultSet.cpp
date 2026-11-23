#include "MysqlCppResultSet.h"
#include "MysqlCppExecution.h"
#include "MysqlCppPrepareStatement.h"
#include "MysqlCppHelper.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#include "mysql-connector-cpp/cppconn/prepared_statement.h"
#include "mysql-connector-cpp/cppconn/resultset.h"
#include "mysql-connector-cpp/cppconn/resultset_metadata.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

MysqlCppResultSet::MysqlCppResultSet(const MysqlCppExecution& execution,
    const std::shared_ptr<MysqlCppPrepareStatement>& statement) :
m_resultSet(nullptr),
m_pendingStatement(nullptr)
{
    if (statement && execution.m_statement && execution.m_statement->execute())
    {
        m_statement = statement;
        sql::ResultSetMetaData* metadata = execution.m_statement->getMetaData();
        if (metadata && metadata->getColumnCount() == 0)
        {
            m_pendingStatement = execution.m_statement;
            m_pendingSession = execution.m_session;
        }
        m_resultSet = execution.m_statement->getResultSet();
    }
}

MysqlCppResultSet::~MysqlCppResultSet()
{
    close();
}

bool MysqlCppResultSet::close()
{
    delete m_resultSet;
    m_resultSet = nullptr;
    bool success = true;
    if (m_pendingStatement && !m_pendingSession.expired())
    {
        // CALL 在首组行后还会发送结束状态，也可能继续返回行；先释放各组再取下一响应。
        try
        {
            while (m_pendingStatement->getMoreResults())
            {
                std::unique_ptr<sql::ResultSet> remaining(m_pendingStatement->getResultSet());
            }
        }
        catch (...)
        {
            success = false;
        }
    }
    m_pendingStatement = nullptr;
    m_pendingSession.reset();
    m_statement.reset();
    return success;
}

bool MysqlCppResultSet::isValid() const
{
    return m_resultSet != nullptr && m_statement && m_statement->isValid();
}

std::vector<std::vector<std::string>> MysqlCppResultSet::toVector()
{
    std::vector<std::vector<std::string>> result;
    int32_t column = columnCount();
    while (next())
    {
        std::vector<std::string> row;
        for (int32_t i = 0; i < column; ++i)
        {
            row.push_back(getString(static_cast<uint32_t>(i)));
        }
        result.push_back(row);
    }
    return result;
}

std::vector<std::map<std::string, std::string>> MysqlCppResultSet::toMapVector()
{
    std::vector<std::map<std::string, std::string>> result;
    std::vector<std::string> names = columnNames();
    while (next())
    {
        std::map<std::string, std::string> row;
        for (size_t i = 0; i < names.size(); ++i)
        {
            row[names[i]] = getString(static_cast<uint32_t>(i));
        }
        result.push_back(row);
    }
    return result;
}

int32_t MysqlCppResultSet::columnCount()
{
    if (!isValid())
    {
        return 0;
    }
    try
    {
        sql::ResultSetMetaData* metaData = m_resultSet->getMetaData();
        if (metaData == nullptr)
        {
            return 0;
        }
        return static_cast<int32_t>(metaData->getColumnCount());
    }
    catch (...)
    {
        return 0;
    }
}

std::vector<std::string> MysqlCppResultSet::columnNames() const
{
    std::vector<std::string> names;
    if (!isValid())
    {
        return names;
    }
    try
    {
        sql::ResultSetMetaData* metaData = m_resultSet->getMetaData();
        if (metaData == nullptr)
        {
            return names;
        }
        uint32_t count = metaData->getColumnCount();
        for (uint32_t i = 1; i <= count; ++i)
        {
            std::string label = metaData->getColumnLabel(i).c_str();
            if (label.empty())
            {
                label = metaData->getColumnName(i).c_str();
            }
            names.push_back(label);
        }
    }
    catch (...)
    {
        names.clear();
    }
    return names;
}

uint32_t MysqlCppResultSet::findColumnIndex(const std::string& label) const
{
    if (!isValid() || label.find('\0') != std::string::npos)
    {
        return UINT32_MAX;
    }
    try
    {
        const uint32_t index = m_resultSet->findColumn(label);
        return index == 0 ? UINT32_MAX : index - 1;
    }
    catch (...)
    {
        return UINT32_MAX;
    }
}

std::istream* MysqlCppResultSet::getBlob(uint32_t columnIndex) const
{
    if (!isValid())
    {
        return nullptr;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(columnIndex);
    if (driverIndex == 0)
    {
        return nullptr;
    }
    try
    {
        return m_resultSet->getBlob(driverIndex);
    }
    catch (...)
    {
        return nullptr;
    }
}

std::istream* MysqlCppResultSet::getBlob(const std::string& columnLabel) const
{
    return getBlob(findColumnIndex(columnLabel));
}

bool MysqlCppResultSet::getBoolean(uint32_t columnIndex) const
{
    if (!isValid())
    {
        return false;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(columnIndex);
    if (driverIndex == 0)
    {
        return false;
    }
    try
    {
        return m_resultSet->getBoolean(driverIndex);
    }
    catch (...)
    {
        return false;
    }
}

bool MysqlCppResultSet::getBoolean(const std::string& columnLabel) const
{
    return getBoolean(findColumnIndex(columnLabel));
}

long double MysqlCppResultSet::getDouble(uint32_t columnIndex) const
{
    if (!isValid())
    {
        return 0;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(columnIndex);
    if (driverIndex == 0)
    {
        return 0;
    }
    try
    {
        return m_resultSet->getDouble(driverIndex);
    }
    catch (...)
    {
        return 0;
    }
}

long double MysqlCppResultSet::getDouble(const std::string& columnLabel) const
{
    return getDouble(findColumnIndex(columnLabel));
}

int32_t MysqlCppResultSet::getInt(uint32_t columnIndex) const
{
    if (!isValid())
    {
        return 0;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(columnIndex);
    if (driverIndex == 0)
    {
        return 0;
    }
    try
    {
        return m_resultSet->getInt(driverIndex);
    }
    catch (...)
    {
        return 0;
    }
}

int32_t MysqlCppResultSet::getInt(const std::string& columnLabel) const
{
    return getInt(findColumnIndex(columnLabel));
}

uint32_t MysqlCppResultSet::getUInt(uint32_t columnIndex) const
{
    if (!isValid())
    {
        return 0;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(columnIndex);
    if (driverIndex == 0)
    {
        return 0;
    }
    try
    {
        return m_resultSet->getUInt(driverIndex);
    }
    catch (...)
    {
        return 0;
    }
}

uint32_t MysqlCppResultSet::getUInt(const std::string& columnLabel) const
{
    return getUInt(findColumnIndex(columnLabel));
}

int64_t MysqlCppResultSet::getInt64(uint32_t columnIndex) const
{
    if (!isValid())
    {
        return 0;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(columnIndex);
    if (driverIndex == 0)
    {
        return 0;
    }
    try
    {
        return m_resultSet->getInt64(driverIndex);
    }
    catch (...)
    {
        return 0;
    }
}

int64_t MysqlCppResultSet::getInt64(const std::string& columnLabel) const
{
    return getInt64(findColumnIndex(columnLabel));
}

uint64_t MysqlCppResultSet::getUInt64(uint32_t columnIndex) const
{
    if (!isValid())
    {
        return 0;
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(columnIndex);
    if (driverIndex == 0)
    {
        return 0;
    }
    try
    {
        return m_resultSet->getUInt64(driverIndex);
    }
    catch (...)
    {
        return 0;
    }
}

uint64_t MysqlCppResultSet::getUInt64(const std::string& columnLabel) const
{
    return getUInt64(findColumnIndex(columnLabel));
}

std::string MysqlCppResultSet::getString(uint32_t columnIndex) const
{
    if (!isValid())
    {
        return "";
    }
    uint32_t driverIndex = MysqlCppHelper::toDriverIndex(columnIndex);
    if (driverIndex == 0)
    {
        return "";
    }
    try
    {
        return m_resultSet->getString(driverIndex);
    }
    catch (...)
    {
        return "";
    }
}

std::string MysqlCppResultSet::getString(const std::string& columnLabel) const
{
    return getString(findColumnIndex(columnLabel));
}

bool MysqlCppResultSet::next()
{
    if (!isValid())
    {
        return false;
    }
    try
    {
        return m_resultSet->next();
    }
    catch (...)
    {
        return false;
    }
}

bool MysqlCppResultSet::previous()
{
    if (!isValid())
    {
        return false;
    }
    try
    {
        return m_resultSet->previous();
    }
    catch (...)
    {
        return false;
    }
}

size_t MysqlCppResultSet::rowsCount()
{
    if (!isValid())
    {
        return 0;
    }
    try
    {
        return m_resultSet->rowsCount();
    }
    catch (...)
    {
        return 0;
    }
}