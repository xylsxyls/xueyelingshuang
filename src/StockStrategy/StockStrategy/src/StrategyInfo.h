#pragma once
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <map>
#include <memory>
#include "StockStrategyMacro.h"

class StockMarket;
struct StockStrategyAPI StrategyInfo
{
	virtual ~StrategyInfo(){}
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<StockMarket> m_spMarket;
	std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>>* m_allBuyInfo;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	StrategyInfo()
	{
		m_spMarket = nullptr;
		m_allBuyInfo = nullptr;
	}
};