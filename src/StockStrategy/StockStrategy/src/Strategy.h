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

	SAR_RISE_BACK_COUNT,

	SAR_RISE_BACK,

	CATCH_UP,

	SAR_RISE_BACK_THIRTY_LINE,

	LINE_BACK,

	STRATEGY_TYPE_SIZE
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
		const std::shared_ptr<StrategyInfo>& strategyInfo);

	/** 获取需要的zhibiao
	@return 返回需要的zhibiao
	*/
	virtual std::set<std::string> needIndicator();

	/** 获取策略类型
	@return 返回策略类型
	*/
	StrategyType type();

protected:
	StrategyType m_strategyType;
};