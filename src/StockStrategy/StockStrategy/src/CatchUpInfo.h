#pragma once
#include "StrategyInfo.h"
#include <memory>
#include <set>

struct StockStrategyAPI CatchUpInfo : public StrategyInfo
{
	virtual int32_t minPollSize();

	virtual int32_t popSize(int32_t buySize);

	virtual int32_t stockNum();
};