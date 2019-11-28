#pragma once
#include "BigNumber/BigNumberAPI.h"
#include <vector>
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>
#include "StockStrategyMacro.h"
#include "StrategyInfo.h"

enum StrategyType
{
	STRATEGY_INIT,

	SAR_RISE_BACK
};

/** 策略
*/
class StockStrategyAPI Strategy
{
public:
	/** 构造函数
	*/
	Strategy();

public:
	/** 单个gupiao是否需要mairu
	@param [in] date 日期
	@param [out] price 价格
	@param [out] percent 百分比0-100
	@param [out] score 分数
	@param [in] strategyInfo 策略需要的信息
	@return 是否需要mairu
	*/
	virtual bool buy(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		BigNumber& score,
		const std::shared_ptr<StrategyInfo>& strategyInfo) = 0;

	/** 单个gupiao是否需要maichu
	@param [in] date 日期
	@param [out] price 价格
	@param [out] percent 百分比0-100
	@param [out] score 分数
	@param [in] strategyInfo 策略需要的信息
	@return 是否需要maichu
	*/
	virtual bool sell(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		BigNumber& score,
		const std::shared_ptr<StrategyInfo>& strategyInfo) = 0;

	/** 获取gupiaochi个数，该函数需在运行buy之后使用
	@return 返回gupiaochi个数
	*/
	virtual int32_t buyPollSize();

	/** 获取策略类型
	@return 返回策略类型
	*/
	StrategyType type();

protected:
	int32_t m_buyPollSize;
	StrategyType m_strategyType;
};