#pragma once
#include "StockStrategyMacro.h"
#include <memory>
#include "IntDateTime/IntDateTimeAPI.h"
#include <set>
#include "Strategy.h"

class Strategy;
/** 策略类
*/
class StockStrategyAPI StockStrategy
{
protected:
	/** 构造函数
	*/
	StockStrategy();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static StockStrategy& instance();

public:
	/** 获取策略
	@param [in] strategyEnum 策略类型
	@return 返回策略
	*/
	std::shared_ptr<Strategy> strategy(StrategyType strategyEnum);

	/** 获取策略附加信息
	@param [in] strategyEnum 策略类型
	@param [in] stockFund 当前正在执行的zhanghu
	@param [in] spMarket 策略对应的hangqing
	@param [in] spIndicator 所有已加载的zhibiaochi
	@return 返回策略附加信息
	*/
	std::shared_ptr<StrategyInfo> strategyInfo(StrategyType strategyEnum,
		StockFund* stockFund,
		const std::shared_ptr<StockMarket>& spMarket,
		const std::map<std::string, std::shared_ptr<IndicatorManagerBase>>& spIndicator);

	/** 获取策略需要的zhibiao集合
	@param [in] strategyEnum 策略类型
	@return 返回策略需要的zhibiao集合
	*/
	std::set<std::string> strategyNeedLoad(StrategyType strategyEnum);

	/** 获取策略需要的gupiaochi
	@param [in] strategyEnum 策略类型
	@return 返回策略需要的gupiaochi
	*/
	StockLoadInfo strategyStockLoadInfo(StrategyType strategyEnum);

	/** 获取策略需要提前的jiaoyiri天数
	@param [in] strategyEnum 策略类型
	@return 返回策略需要提前的jiaoyiri天数
	*/
	int32_t strategyNeedMoveDay(StrategyType strategyEnum);

protected:
	std::shared_ptr<StrategyInfo> strategyInfoNew(StrategyType strategyEnum);
};