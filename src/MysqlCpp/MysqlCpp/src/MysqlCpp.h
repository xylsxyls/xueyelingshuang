#pragma once
#include "MysqlCppMacro.h"
#include "MysqlCppResultSet.h"
#include "MysqlCppPrepareStatement.h"
#include <memory>

namespace sql
{
	class Driver;
	class Connection;
}

class MysqlCppPrepareStatement;

class MysqlCppAPI MysqlCpp
{
public:
	/** 构造函数
	*/
	MysqlCpp();

	/** 析构函数
	*/
	~MysqlCpp();

public:
	/** 连接数据库
	@param [in] ip IP地址
	@param [in] port 端口
	@param [in] account 账号
	@param [in] password 密码
	@param [in] dbName 表名
	@return 返回连接是否成功
	*/
	bool connect(const std::string& ip,
				 int32_t port,
				 const std::string& account,
				 const std::string& password);

	/** 选择数据库
	@param [in] dbName 数据库名
	@param [in] autoCommit 是否自动提交
	*/
	void selectDb(const std::string& dbName, bool autoCommit = false) const;

	/** 创建一个用于增删改查的类
	@param [in] sqlString sql字符串
	@return 返回操作条件语句的类
	*/
	std::shared_ptr<MysqlCppPrepareStatement> PreparedStatementCreator(const std::string& sqlString) const;

	/** 操作查
	@param [in] statement 条件语句
	@return 返回查询结果
	*/
	std::shared_ptr<MysqlCppResultSet> execute(const std::shared_ptr<MysqlCppPrepareStatement>& statement) const;

	/** 导入
	@param [in] sqlPath 导入的sql文件路径
	@param [in] host 主机IP地址
	@param [in] user 用户名
	@param [in] password 密码
	@param [in] database 数据库名
	*/
	void importSql(const std::string sqlPath,
				   const std::string& host,
				   const std::string& user,
				   const std::string& password,
				   const std::string& database) const;

	/** 导出
	@param [in] sqlPath 导入的sql文件路径
	@param [in] host 主机IP地址
	@param [in] user 用户名
	@param [in] password 密码
	@param [in] exportData 导出内容，database或者database table
	*/
	void exportSql(const std::string sqlPath,
				   const std::string& host,
				   const std::string& user,
				   const std::string& password,
				   const std::string& exportData) const;

	/** 设置是否自动提交
	@param [in] autoCommit 是否自动提交
	*/
	void setAutoCommit(bool autoCommit) const;

	/** 提交
	*/
	void commit() const;

	/** 回滚
	*/
	void rollback() const;

protected:
	bool check() const;

public:
	sql::Driver* m_driver;
	sql::Connection* m_con;
};