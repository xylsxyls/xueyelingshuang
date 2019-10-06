#pragma once
#include "StockTradeMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <map>

class StockTradeAPI StockTrade
{
public:
	void init(const IntDateTime& beginTime, const IntDateTime& endTime, const std::vector<std::string>& allStock, StrategyEnum strategyEnum);

	void load();

	bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock, const IntDateTime& date);

	bool sell(const std::string& stock, const IntDateTime& date, BigNumber& price, BigNumber& percent);

private:
	IntDateTime m_beginTime;
	IntDateTime m_endTime;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<std::string, std::shared_ptr<Strategy>> m_stockStrategyMap;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};