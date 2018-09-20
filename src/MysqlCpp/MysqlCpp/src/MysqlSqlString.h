#pragma once
#include "MysqlCppMacro.h"
#include "SqlString/SqlStringAPI.h"

class MysqlCppAPI MysqlSqlString : public SqlString
{
public:
	static std::string importString(const std::string& sqlPath,
									const std::string& host,
									const std::string& user,
									const std::string& password,
									const std::string& database);

	static std::string exportString(const std::string& sqlPath,
									const std::string& host,
									const std::string& user,
									const std::string& password,
									const std::string& exportData);
};