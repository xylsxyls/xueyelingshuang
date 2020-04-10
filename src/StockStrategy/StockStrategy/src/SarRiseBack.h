#pragma once
#include "Strategy.h"
#include <memory>
#include "StrategyInfo.h"

class StockStrategyAPI SarRiseBack : public Strategy
{
public:
	/** 构造函数
	*/
	SarRiseBack();

public:
	/** 单个gupiao是否需要mairu
	@param [in] date 日期
	@param [in] stockInfo 选中信息
	@return 是否需要mairu
	*/
	virtual bool buy(const IntDateTime& date, StockInfo& stockInfo);

	/** 单个gupiao是否需要maichu
	@param [in] date 日期
	@param [in] stockInfo 选中信息
	@return 是否需要maichu
	*/
	virtual bool sell(const IntDateTime& date, StockInfo& stockInfo);

	/** 观察类型单个gupiao是否需要maichu
	@param [in] date 日期
	@param [in] stockInfo 选中信息
	@return 是否需要maichu
	*/
	virtual bool observeSell(const IntDateTime& date, StockInfo& stockInfo);
};