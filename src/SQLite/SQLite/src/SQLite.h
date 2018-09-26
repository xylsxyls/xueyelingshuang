#pragma once
#include "SQLiteMacro.h"
#include <string>
#include <memory>

class QSqlDatabase;
class SQLitePrepareStatement;
class SQLiteResultSet;

class SQLiteAPI SQLite
{
public:
	enum PragmaFlag
	{
		/** 最安全的方式但是最慢
		*/
		PRAGMA_FULL = 2,

		/** 硬盘不可用时数据库文件也会损坏
		*/
		PRAGMA_NORMAL = 1,

		/** 没有保护机制，但是程序崩溃不会影响文件
		*/
		PRAGMA_OFF = 0
	};
public:
	/** 构造函数
	@param [in] dbName 数据库文件路径
	@param [in] pragmaFlag 数据库存储方式
	*/
	SQLite(const std::string& dbFilePath, PragmaFlag pragmaFlag = PRAGMA_OFF);

	/** 析构函数
	*/
	~SQLite();

public:
	/** 创建一个用于增删改查的类
	@param [in] sqlString sql字符串
	@return 返回操作条件语句的类
	*/
	std::shared_ptr<SQLitePrepareStatement> preparedCreator(const std::string& sqlString);

	/** 执行语句
	@param [in] spPrepareQuery creator创建的类
	@return 返回执行结果	
	*/
	std::shared_ptr<SQLiteResultSet> execute(const std::shared_ptr<SQLitePrepareStatement>& spPrepareQuery);

	/** 开启事务
	*/
	void transaction();

	/** 提交
	*/
	void commit();

	/** 回滚
	*/
	void rollback();

private:
	QSqlDatabase* m_db;
};