#include "MysqlSqlString.h"
#include "MysqlCppHelper.h"

/** 检查参数能否作为当前系统的命令文本片段；实际导入导出直接启动工具
@param [in] text 工具路径、SQL 路径或一个参数
@return Windows shell 不能无损表示或包含零字节时返回 false
*/
static bool CanFormatArgument(const std::string& text)
{
#ifdef _WIN32
    if (text.find_first_of("\"%!\r\n") != std::string::npos)
    {
        return false;
    }
#endif
    return text.find('\0') == std::string::npos;
}

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
    if (!CanFormatArgument(tool) || !CanFormatArgument(sqlPath) || !CanFormatArgument(host) ||
        !CanFormatArgument(user) || !CanFormatArgument(password) || !CanFormatArgument(database))
    {
        return std::string();
    }
    return MysqlCppHelper::quoteCommandText(tool) + " -h" + MysqlCppHelper::quoteCommandText(host) +
        " -u" + MysqlCppHelper::quoteCommandText(user) + " -p" + MysqlCppHelper::quoteCommandText(password) + " -- " +
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
    if (!CanFormatArgument(tool) || !CanFormatArgument(sqlPath) || !CanFormatArgument(host) ||
        !CanFormatArgument(user) || !CanFormatArgument(password) || !CanFormatArgument(exportData))
    {
        return std::string();
    }
    return MysqlCppHelper::quoteCommandText(tool) + " -h" + MysqlCppHelper::quoteCommandText(host) +
        " -u" + MysqlCppHelper::quoteCommandText(user) + " -p" + MysqlCppHelper::quoteCommandText(password) + " -- " +
        MysqlCppHelper::quoteCommandText(exportData) + " > " + MysqlCppHelper::quoteCommandText(sqlPath);
}