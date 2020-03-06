#pragma once
#include "Strategy.h"
#include <memory>
#include "StrategyInfo.h"

class StockStrategyAPI SarRiseBackCount : public Strategy
{
public:
	/** 构造函数
	*/
	SarRiseBackCount();

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
		const std::shared_ptr<StrategyInfo>& strategyInfo);

	/** 获取需要的zhibiao
	@return 返回需要的zhibiao
	*/
	virtual std::set<std::string> needIndicator();
};