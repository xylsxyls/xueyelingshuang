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
};