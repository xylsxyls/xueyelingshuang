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

/** 生成使用 TCP 主机的 MySQL 工具命令展示文本
@param [in] input true 生成导入命令，false 生成导出命令
@param [in] sqlPath 输入或输出文件路径
@param [in] host 与连接配置格式相同的主机
@param [in] user 数据库账号
@param [in] password 展示文本将包含的密码
@param [in] database 单个数据库名
@return 无法格式化时为空串，否则为带重定向的展示文本
*/
static std::string FormatToolCommand(bool input, const std::string& sqlPath, const std::string& host,
    const std::string& user, const std::string& password, const std::string& database)
{
#ifdef _WIN32
    const char* name = input ? "mysql.exe" : "mysqldump.exe";
#else
    const char* name = input ? "mysql" : "mysqldump";
#endif
    const std::string tool = MysqlCppHelper::mysqlToolPath(name);
    const std::string address = MysqlCppHelper::toolHost(host);
    if (address.empty() || !CanFormatArgument(tool) || !CanFormatArgument(sqlPath) || !CanFormatArgument(address) ||
        !CanFormatArgument(user) || !CanFormatArgument(password) || !CanFormatArgument(database))
    {
        return std::string();
    }
    return MysqlCppHelper::quoteCommandText(tool) + " -h" + MysqlCppHelper::quoteCommandText(address) +
        " -u" + MysqlCppHelper::quoteCommandText(user) + " -p" + MysqlCppHelper::quoteCommandText(password) + " -- " +
        MysqlCppHelper::quoteCommandText(database) + (input ? " < " : " > ") + MysqlCppHelper::quoteCommandText(sqlPath);
}

std::string MysqlSqlString::importString(const std::string& sqlPath, const std::string& host,
    const std::string& user, const std::string& password, const std::string& database)
{
    return FormatToolCommand(true, sqlPath, host, user, password, database);
}

std::string MysqlSqlString::exportString(const std::string& sqlPath, const std::string& host,
    const std::string& user, const std::string& password, const std::string& exportData)
{
    return FormatToolCommand(false, sqlPath, host, user, password, exportData);
}