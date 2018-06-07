#include "MysqlCppResultSet.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#include "cppconn/resultset.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

MysqlCppResultSet::MysqlCppResultSet(sql::ResultSet* resultSet):
m_resultSet(resultSet)
{

}

MysqlCppResultSet::~MysqlCppResultSet()
{
	delete m_resultSet;
}

std::vector<std::vector<std::string>> MysqlCppResultSet::toVector()
{
	std::vector<std::vector<std::string>> result;
	int32_t column = columnCount();
	while (next())
	{
		std::vector<std::string> vecRow;
		int32_t index = 0;
		while (index++ != column)
		{
			vecRow.push_back(getString(index));
		}
		result.push_back(vecRow);
	}
	return result;
}

int32_t MysqlCppResultSet::columnCount()
{
	int32_t columnCount = 0;
	bool hasNext = next();
	if (hasNext == false)
	{
		return 0;
	}
	while (true)
	{
		try
		{
			m_resultSet->getString(++columnCount);
		}
		catch (...)
		{
			break;
		}
	}
	--columnCount;
	previous();
	return columnCount;
}

std::istream* MysqlCppResultSet::getBlob(uint32_t columnIndex) const
{
	if (m_resultSet == nullptr)
	{
		return nullptr;
	}
	try
	{
		return m_resultSet->getBlob(columnIndex);
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
	try
	{
		return m_resultSet->getBoolean(columnIndex);
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
	try
	{
		return m_resultSet->getDouble(columnIndex);
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
	try
	{
		return m_resultSet->getInt(columnIndex);
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
	try
	{
		return m_resultSet->getUInt(columnIndex);
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
	try
	{
		return m_resultSet->getInt64(columnIndex);
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
	try
	{
		return m_resultSet->getUInt64(columnIndex);
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
	try
	{
		return m_resultSet->getString(columnIndex);
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

size_t MysqlCppResultSet::rowsCount() const
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