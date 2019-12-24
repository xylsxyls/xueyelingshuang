#pragma once
#include "StockIndicatorMacro.h"
#include <memory>
#include <map>
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockMarket/StockMarketAPI.h"

class StockWrIndicator;
class StockRsiIndicator;
class StockSarIndicator;
class StockBollIndicator;
class StockAvgIndicator;
struct StockAvg;

/** zhibiao管理类
*/
class StockIndicatorAPI StockIndicator
{
protected:
	/** 构造函数
	*/
	StockIndicator();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static StockIndicator& instance();

public:
	/** 从redis中加载原始数据，加载完后需要立刻通过具体zhibiao接口获取使用
	@param [in] stock gupiao代码
	@param [in] beginTime 开始时间
	@param [in] endTime 结束时间
	*/
	void loadIndicatorFromRedis(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

	/** 从redis中加载原始数据，加载完后需要立刻通过具体zhibiao接口获取使用
	@param [in] stock gupiao代码
	@param [in] beginTime 开始时间
	@param [in] endTime 结束时间
	*/
	void loadCalcFromRedis(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

	/** 清空
	*/
	void clear();

	/** 存储wr
	*/
	void saveWr();

	/** 存储rsi
	*/
	void saveRsi();

	/** 存储sar
	*/
	void saveSar();

	/** 存储boll
	*/
	void saveBoll();

	/** 计算某一天的wr
	@param [in] date 日期
	@param [out] indicatorData 数据
	@param [in] allStock 所有gupiao
	@param [in] vecThreadId 计算线程id
	*/
	void dateIndicator(const IntDateTime& date,
		std::map<std::string, std::vector<std::vector<std::string>>>& wrIndicatorData,
		std::map<std::string, std::vector<std::vector<std::string>>>& rsiIndicatorData,
		std::map<std::string, std::vector<std::vector<std::string>>>& sarIndicatorData,
		std::map<std::string, std::vector<std::vector<std::string>>>& bollIndicatorData,
		const std::vector<std::string>& allStock,
		bool loadFromMysql,
		const std::vector<uint32_t>& vecThreadId);

	/** 存储某一天的zhibiao
	@param [in] date 日期
	*/
	void saveDateIndicator(const IntDateTime& date);

	/** 存储avg
	@param [in] stock gupiao代码
	@param [in] avgData junxian数据
	*/
	void saveAvg(const std::string& stock, const std::map<IntDateTime, std::shared_ptr<StockAvg>>& avgData);

	/** 更新某一天的zhibiao到redis
	@param [in] date 日期
	@param [in] useLast 如果上次有值就用上次的
	*/
	void updateDateIndicatorToRedis(const IntDateTime& date, bool useLast);

	/** 获取wr相关接口
	@return 返回wr相关接口
	*/
	std::shared_ptr<StockWrIndicator> wr();

	/** 获取rsi相关接口
	@return 返回rsi相关接口
	*/
	std::shared_ptr<StockRsiIndicator> rsi();

	/** 获取sar相关接口
	@return 返回sar相关接口
	*/
	std::shared_ptr<StockSarIndicator> sar();

	/** 获取boll相关接口
	@return 返回boll相关接口
	*/
	std::shared_ptr<StockBollIndicator> boll();

	/** 获取avg相关接口
	@return 返回avg相关接口
	*/
	std::shared_ptr<StockAvgIndicator> avg();

protected:
	std::vector<std::vector<std::string>> split(const std::vector<std::string>& allStock, int32_t num = 1000);

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_loadStock;
	std::map<std::string, std::vector<int32_t>> m_indicatorIndex;
	std::shared_ptr<std::vector<std::vector<std::string>>> m_redisIndicatorData;
	std::map<std::string, std::vector<int32_t>> m_calcIndex;
	std::shared_ptr<std::vector<std::vector<std::string>>> m_redisCalcData;

	std::map<std::string, std::vector<std::vector<std::string>>> m_wrIndicatorData;
	std::map<std::string, std::vector<std::vector<std::string>>> m_rsiIndicatorData;
	std::map<std::string, std::vector<std::vector<std::string>>> m_sarIndicatorData;
	std::map<std::string, std::vector<std::vector<std::string>>> m_bollIndicatorData;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};