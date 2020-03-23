#pragma once
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <map>
#include <memory>
#include <set>
#include "StockStrategyMacro.h"
#include "StockIndicator/StockIndicatorAPI.h"

class StockFund;
class StockMarket;
struct StockStrategyAPI StrategyInfo
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<StockMarket> m_spMarket;
	std::map<std::string, std::shared_ptr<IndicatorManagerBase>> m_spIndicator;
	StockFund* m_fund;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	StrategyInfo()
	{
		m_spMarket = nullptr;
		m_fund = nullptr;
	}

	virtual ~StrategyInfo()
	{

	}

	virtual int32_t minPollSize() = 0;

	virtual int32_t popSize(int32_t buySize) = 0;

	virtual int32_t stockNum() = 0;
};