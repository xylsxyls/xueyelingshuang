#include "MysqlCppResultSet.h"
#include "MysqlCppHelper.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#include "cppconn/resultset.h"
#include "cppconn/resultset_metadata.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

MysqlCppResultSet::MysqlCppResultSet(sql::ResultSet* resultSet) :
m_resultSet(resultSet)
{

}

MysqlCppResultSet::~MysqlCppResultSet()
{
    delete m_resultSet;
}

bool MysqlCppResultSet::isValid() const
{
    return m_resultSet != nullptr;
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
    if (m_resultSet == nullptr)
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
    if (m_resultSet == nullptr)
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

std::istream* MysqlCppResultSet::getBlob(uint32_t columnIndex) const
{
    if (m_resultSet == nullptr)
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
    if (m_resultSet == nullptr)
    {
        return nullptr;
    }
    try
    {
        return m_resultSet->getBlob(columnLabel);
    }
    catch (...)
    {
        return nullptr;
    }
}

bool MysqlCppResultSet::getBoolean(uint32_t columnIndex) const
{
    if (m_resultSet == nullptr)
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
    if (m_resultSet == nullptr)
    {
        return false;
    }
    try
    {
        return m_resultSet->getBoolean(columnLabel);
    }
    catch (...)
    {
        return false;
    }
}

long double MysqlCppResultSet::getDouble(uint32_t columnIndex) const
{
    if (m_resultSet == nullptr)
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
    if (m_resultSet == nullptr)
    {
        return 0;
    }
    try
    {
        return m_resultSet->getDouble(columnLabel);
    }
    catch (...)
    {
        return 0;
    }
}

int32_t MysqlCppResultSet::getInt(uint32_t columnIndex) const
{
    if (m_resultSet == nullptr)
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
    if (m_resultSet == nullptr)
    {
        return 0;
    }
    try
    {
        return m_resultSet->getInt(columnLabel);
    }
    catch (...)
    {
        return 0;
    }
}

uint32_t MysqlCppResultSet::getUInt(uint32_t columnIndex) const
{
    if (m_resultSet == nullptr)
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
    if (m_resultSet == nullptr)
    {
        return 0;
    }
    try
    {
        return m_resultSet->getUInt(columnLabel);
    }
    catch (...)
    {
        return 0;
    }
}

int64_t MysqlCppResultSet::getInt64(uint32_t columnIndex) const
{
    if (m_resultSet == nullptr)
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
    if (m_resultSet == nullptr)
    {
        return 0;
    }
    try
    {
        return m_resultSet->getInt64(columnLabel);
    }
    catch (...)
    {
        return 0;
    }
}

uint64_t MysqlCppResultSet::getUInt64(uint32_t columnIndex) const
{
    if (m_resultSet == nullptr)
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
    if (m_resultSet == nullptr)
    {
        return 0;
    }
    try
    {
        return m_resultSet->getUInt64(columnLabel);
    }
    catch (...)
    {
        return 0;
    }
}

std::string MysqlCppResultSet::getString(uint32_t columnIndex) const
{
    if (m_resultSet == nullptr)
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
    if (m_resultSet == nullptr)
    {
        return "";
    }
    try
    {
        return m_resultSet->getString(columnLabel);
    }
    catch (...)
    {
        return "";
    }
}

bool MysqlCppResultSet::next()
{
    if (m_resultSet == nullptr)
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
    if (m_resultSet == nullptr)
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
    if (m_resultSet == nullptr)
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