#pragma once
#include "StrategyInfo.h"
#include <memory>

struct StockStrategyAPI T0Info : public StrategyInfo
{
	/** 最小通过个数
	@return 返回最小通过个数
	*/
	virtual int32_t minPollSize();

	/** 过滤个数
	@return 返回过滤个数
	*/
	virtual int32_t popSize(int32_t buySize);

	/** 选择个数
	@return 返回选择个数
	*/
	virtual int32_t stockNum();

	/** 获取需要的zhibiao
	@return 返回需要的zhibiao
	*/
	virtual std::set<std::string> needIndicator();

	/** 需要的gupiaochi
	@return 返回需要的gupiaochi
	*/
	virtual StockLoadInfo needStockLoadInfo();

	/** 获取需要提前的jiaoyiri天数
	@return 返回需要提前的jiaoyiri天数
	*/
	virtual int32_t needMoveDay();
};