#pragma once
#include "StockMysqlMacro.h"
#include "MysqlCpp/MysqlCppAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"

/** 数据库对接类
*/
class StockMysqlAPI StockMysql
{
protected:
	/** 构造函数
	*/
	StockMysql();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static StockMysql& instance();

	/** 初始化
	*/
	void init();

public:
	/** 获取所有股票代码
	@return 返回所有股票代码
	*/
	std::vector<std::string> allStock();

	/** 新开一个开高低收的表
	@param [in] stock 股票代码
	*/
	void createMarketHead(const std::string& stock);

	/** 添加一个股票代码到股票集合
	@param [in] stock 股票代码
	*/
	void addStock(const std::string& stock);

	/** 存储开高低收
	@param [in] stock 股票代码
	@param [in] vecMarket 股票代码
	*/
	void saveMarket(const std::string& stock, const std::vector<std::vector<std::string>>& vecMarket);

	/** 读取开高低收
	@param [in] stock 股票代码
	@param [in] beginTime 开始时间，默认参数表示最早时间
	@param [in] endTime 结束时间，默认参数表示最晚时间
	@return 返回开高低收表
	*/
	std::vector<std::vector<std::string>> readMarket(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

private:
	//数据库连接类
	MysqlCpp m_mysql;
};