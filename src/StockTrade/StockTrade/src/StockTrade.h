#pragma once
#include "StockTradeMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <map>
#include <memory>

class StockDay;
/** gupiaojiaoyi策略
*/
class StockTradeAPI StockTrade
{
public:
	/** 初始化
	@param [in] beginTime 开始时间
	@param [in] endTime 结束时间
	@param [in] allStock gupiaochi
	@param [in] strategyEnum 策略类型
	*/
	void init(const IntDateTime& beginTime,
		const IntDateTime& endTime,
		const std::vector<std::string>& allStock,
		StrategyEnum strategyEnum);

	/** 原始信息加载转化到可用信息
	*/
	void load();

	/** 选出可以goumai的gupiao
	@param [out] buyStock 选出的gupiao集合，stock,price,percent
	@param [in] date 日期
	@param [in] allBuyInfo 所有的已goumai信息
	@return 是否有选出的gupiao
	*/
	bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>>& allBuyInfo);

	/** 询问单只gupiao需不需要maichu
	@param [in] stock gupiao代码
	@param [in] date 日期
	@param [out] price maichu价格
	@param [out] percent maichu比例，0-100
	@param [in] buyInfo goumai信息
	@return 返回询问的gupiao需不需要maichu
	*/
	bool sell(const std::string& stock,
		const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		const std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& buyInfo);

	/** 获取内置的hangqing信息
	@param [in] stock gupiao代码
	@return 返回一天的hangqing信息
	*/
	std::shared_ptr<StockMarket> market(const std::string& stock);

	/** 获取指定日期指定gupiao集合的最新有效数据集合
	@param [in] vecStock gupiao集合
	@param [in] date 日期
	@param [out] dayData 最新有效数据集合
	@return 返回是否获取成功
	*/
	bool stockDayData(const std::vector<std::string>& vecStock,
		const IntDateTime& date,
		std::map<std::string, std::shared_ptr<StockDay>>& dayData);

private:
	IntDateTime m_beginTime;
	IntDateTime m_endTime;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//stock,strategy
	std::map<std::string, std::shared_ptr<Strategy>> m_stockStrategyMap;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};