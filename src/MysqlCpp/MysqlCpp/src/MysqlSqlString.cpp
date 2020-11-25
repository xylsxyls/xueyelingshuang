#include "MysqlSqlString.h"
#include <stdint.h>
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"

std::string MysqlSqlString::importString(const std::string& sqlPath,
									const std::string& host,
									const std::string& user,
									const std::string& password,
									const std::string& database)
{
	return CStringManager::Format("\"%s\" -h%s -u%s -p%s %s < %s",
		(CSystem::GetEnvironment("MYSQL_ROOT") + "bin\\mysql.exe").c_str(),
		host.c_str(),
		user.c_str(),
		password.c_str(),
		database.c_str(),
		sqlPath.c_str());
}

std::string MysqlSqlString::exportString(const std::string& sqlPath,
									const std::string& host,
									const std::string& user,
									const std::string& password,
									const std::string& exportData)
{
	return CStringManager::Format("\"%s\" -h%s -u%s -p%s %s > %s",
		(CSystem::GetEnvironment("MYSQL_ROOT") + "bin\\mysqldump.exe").c_str(),
		host.c_str(),
		user.c_str(),
		password.c_str(),
		exportData.c_str(),
		sqlPath.c_str());
}