#pragma once
#include "StockMysqlMacro.h"
#include "MysqlCpp/MysqlCppAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"

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
	/** 获取所有gupiao代码
	@return 返回所有gupiao代码
	*/
	std::vector<std::string> allStock() const;

	/** 新开一个kaigaodishou的表
	@param [in] stock gupiao代码
	*/
	void createMarketHead(const std::string& stock);

	/** 添加一个gupiao代码到gupiao集合
	@param [in] stock gupiao代码
	*/
	void addStock(const std::string& stock);

	/** 存储kaigaodishou
	@param [in] stock gupiao代码
	@param [in] vecMarket gupiao代码
	*/
	void saveMarket(const std::string& stock, const std::vector<std::vector<std::string>>& vecMarket);

	/** 读取kaigaodishou
	@param [in] stock gupiao代码
	@param [in] beginTime 开始时间，默认参数表示最早时间
	@param [in] endTime 结束时间，默认参数表示最晚时间
	@return 返回kaigaodishou表
	*/
	std::vector<std::vector<std::string>> readMarket(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0)) const;

	/** 读取wr
	@param [in] stock gupiao代码
	@param [in] beginTime 开始时间，默认参数表示最早时间
	@param [in] endTime 结束时间，默认参数表示最晚时间
	@return 返回wr表
	*/
	std::vector<std::vector<std::string>> readWr(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0)) const;

	/** 读取rsi
	@param [in] stock gupiao代码
	@param [in] beginTime 开始时间，默认参数表示最早时间
	@param [in] endTime 结束时间，默认参数表示最晚时间
	@return 返回wr表
	*/
	std::vector<std::vector<std::string>> readRsi(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0)) const;

	/** 获取当前日期是否存在
	@param [in] stock gupiao代码
	@param [in] date 日期
	@return 返回是否存在
	*/
	bool dateExist(const std::string& stock, const IntDateTime& date) const;

	/** 获取第一个jiaoyiri
	@param [in] stock gupiao代码
	@return 返回第一个jiaoyiri
	*/
	IntDateTime beginDate(const std::string& stock) const;

	/** 获取最后一个jiaoyiri
	@param [in] stock gupiao代码
	@return 返回最后一个jiaoyiri
	*/
	IntDateTime endDate(const std::string& stock) const;

	/** 获取上一个jiaoyiri日期
	@param [in] stock gupiao代码
	@param [in] date 日期
	@return 返回上一个jiaoyiri日期
	*/
	IntDateTime getDatePre(const std::string& stock, const IntDateTime& date) const;

	/** 获取下一个jiaoyiri日期
	@param [in] stock gupiao代码
	@param [in] date 日期
	@return 返回下一个jiaoyiri日期
	*/
	IntDateTime getDateNext(const std::string& stock, const IntDateTime& date) const;

	/** 获取两个日期之间的jiaoyiri天数
	@param [in] stock gupiao代码
	@param [in] date1 jiaoyiri日期1，1小
	@param [in] date2 jiaoyiri日期2，2大
	@return 返回间隔天数，包括头尾
	*/
	BigNumber getDays(const std::string& stock, const IntDateTime& date1, const IntDateTime& date2);

private:
	//数据库连接类
	MysqlCpp m_mysql;
};