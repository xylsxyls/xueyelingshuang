#pragma once
#include "StockRetestMacro.h"
#include <memory>
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include "StockSolution/StockSolutionAPI.h"
#include "StockFund/StockFundAPI.h"
#include "StockTrade/StockTradeAPI.h"

class Strategy;
class StockRetestAPI StockDaysRetest
{
public:
	StockDaysRetest();

	~StockDaysRetest();

public:
	void init(SolutionType solutionType,
		const std::vector<std::pair<StrategyType, StrategyType>>& vecStrategyType,
		const IntDateTime& beginTime,
		const IntDateTime& endTime,
		const BigNumber& initialFund = "20000000000",
		bool showStockLog = false,
		int32_t threadCount = 0);

	void load();

	void run();

	void printProfit(const IntDateTime& currentTime);

private:
	SolutionType m_solutionType;
	bool m_showStockLog;
	IntDateTime m_beginTime;
	IntDateTime m_endTime;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::vector<std::pair<StrategyType, StrategyType>> m_vecStrategyType;
	std::vector<uint32_t> m_vecThreadId;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	uint32_t m_resultThreadId;
	BigNumber m_initialFund;
	BigNumber m_allStockCount;
	StockFund m_fund;
	StockTrade m_trade;
	StockMarket m_runMarket;
};