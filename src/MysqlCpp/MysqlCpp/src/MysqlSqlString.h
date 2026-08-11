#pragma once
#include "MysqlCppMacro.h"
#include <string>

/** mysql命令行导入导出语句构造工具，只负责拼接命令文本，不执行命令
*/
class MysqlCppAPI MysqlSqlString
{
public:
	/** 构造mysql导入SQL文件的命令行字符串
	@param [in] sqlPath SQL文件路径
	@param [in] host MySQL服务器地址
	@param [in] user 登录账号
	@param [in] password 登录密码
	@param [in] database 目标数据库名
	@return 返回可交给系统命令执行的导入命令
	*/
	static std::string importString(const std::string& sqlPath,
									const std::string& host,
									const std::string& user,
									const std::string& password,
									const std::string& database);

	/** 构造mysql导出数据的命令行字符串
	@param [in] sqlPath 导出文件保存路径
	@param [in] host MySQL服务器地址
	@param [in] user 登录账号
	@param [in] password 登录密码
	@param [in] exportData mysqldump后面的导出目标参数，例如数据库名或库表组合
	@return 返回可交给系统命令执行的导出命令
	*/
	static std::string exportString(const std::string& sqlPath,
									const std::string& host,
									const std::string& user,
									const std::string& password,
									const std::string& exportData);
};