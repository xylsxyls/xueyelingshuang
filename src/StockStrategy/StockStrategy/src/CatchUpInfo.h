#pragma once
#include "StrategyInfo.h"
#include <memory>
#include <set>

struct StockStrategyAPI CatchUpInfo : public StrategyInfo
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
};