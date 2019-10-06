#pragma once
#include "StockMarketMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"

class StockMarketAPI StockMarketHelper
{
public:
	/** 存储gupiao历史hangqing
	@param [in] stock gupiao代码
	@param [in] file 下载的原始文件
	*/
	static void saveMarketToMysql(const std::string& stock, const std::string& file);

	/** 更新当天hangqing
	@param [in] stock gupiao代码
	@param [in] date 日期
	@param [in] market 当天hangqing
	*/
	static void updateDateMarketToMysql(const std::string& stock, const IntDateTime& date, const std::vector<std::string>& market);

	/** 更新当天hangqing到redis
	@param [in] stock gupiao代码
	@param [in] date 日期
	@param [in] market 当天hangqing
	*/
	static void updateDateMarketToRedis(const std::string& stock, const IntDateTime& date, const std::vector<std::string>& market);
};