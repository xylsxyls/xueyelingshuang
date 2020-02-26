#pragma once
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <map>
#include <memory>
#include "StockStrategyMacro.h"

class StockFund;
class StockMarket;
struct StockStrategyAPI StrategyInfo
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<StockMarket> m_spMarket;
	StockFund* m_fund;
	int32_t m_minPollSize;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	StrategyInfo()
	{
		m_spMarket = nullptr;
		m_fund = nullptr;
		m_minPollSize = 0;
	}

	virtual ~StrategyInfo()
	{

	}

	virtual int32_t popSize(int32_t buySize) = 0;
};