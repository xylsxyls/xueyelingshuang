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
	@param [in] stockInfo 选中信息
	@return 是否需要mairu
	*/
	virtual bool buy(const IntDateTime& date, StockInfo& stockInfo);
};