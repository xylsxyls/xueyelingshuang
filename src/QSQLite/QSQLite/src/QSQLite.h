#pragma once
#include "QSQLiteMacro.h"
#include <string>
#include <memory>

class QSqlDatabase;
class QSQLitePrepareStatement;
class QSQLiteResultSet;

class QSQLiteAPI QSQLite
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
	*/
	QSQLite(const std::string& dbFilePath);

	/** 析构函数
	*/
	~QSQLite();

public:
	/** 打开数据库
	@param [in] pragmaFlag 数据库存储方式
	*/
	void open(PragmaFlag pragmaFlag = PRAGMA_OFF);

	/** 关闭数据库
	*/
	void close();

	/** 数据库是否已经打开
	@return 返回是否已经打开
	*/
	bool isOpen();

	/** 创建一个用于增删改查的类
	@param [in] sqlString sql字符串
	@return 返回操作条件语句的类
	*/
	QSQLitePrepareStatement preparedCreator(const std::string& sqlString);

	/** 执行语句
	@param [in] spPrepareQuery creator创建的类
	@return 返回执行结果	
	*/
	QSQLiteResultSet execute(QSQLitePrepareStatement& spPrepareQuery);

	/** 开启事务
	*/
	void transaction();

	/** 提交
	*/
	void commit();

	/** 回滚
	*/
	void rollback();

protected:
	void init(const std::string& path);
	void free();

private:
	QSqlDatabase* m_db;
};