#include "MysqlSqlString.h"
#include "MysqlCppHelper.h"

std::string MysqlSqlString::importString(const std::string& sqlPath,
                                         const std::string& host,
                                         const std::string& user,
                                         const std::string& password,
                                         const std::string& database)
{
#ifdef _WIN32
    const std::string tool = MysqlCppHelper::mysqlToolPath("mysql.exe");
#else
    const std::string tool = MysqlCppHelper::mysqlToolPath("mysql");
#endif
    return MysqlCppHelper::quoteCommandText(tool) + " -h" + MysqlCppHelper::quoteCommandText(host) +
        " -u" + MysqlCppHelper::quoteCommandText(user) + " -p" + MysqlCppHelper::quoteCommandText(password) + " " +
        MysqlCppHelper::quoteCommandText(database) + " < " + MysqlCppHelper::quoteCommandText(sqlPath);
}

std::string MysqlSqlString::exportString(const std::string& sqlPath,
                                         const std::string& host,
                                         const std::string& user,
                                         const std::string& password,
                                         const std::string& exportData)
{
#ifdef _WIN32
    const std::string tool = MysqlCppHelper::mysqlToolPath("mysqldump.exe");
#else
    const std::string tool = MysqlCppHelper::mysqlToolPath("mysqldump");
#endif
    return MysqlCppHelper::quoteCommandText(tool) + " -h" + MysqlCppHelper::quoteCommandText(host) +
        " -u" + MysqlCppHelper::quoteCommandText(user) + " -p" + MysqlCppHelper::quoteCommandText(password) + " " +
        MysqlCppHelper::quoteCommandText(exportData) + " > " + MysqlCppHelper::quoteCommandText(sqlPath);
}