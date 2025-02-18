#pragma once
#include "StockRetestMacro.h"
#include <memory>
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockStrategy/StockStrategyAPI.h"

class Strategy;
class StockRetestAPI StockEveryRetest
{
public:
	StockEveryRetest();

	~StockEveryRetest();

public:
	void init(StrategyType strategyType,
		const std::vector<std::string>& allStock,
		const IntDateTime& beginTime,
		const IntDateTime& endTime,
		bool showStockLog = false,
		int32_t threadCount = 0);

	void run();

private:
	StrategyType m_strategyType;
	bool m_showStockLog;
	IntDateTime m_beginTime;
	IntDateTime m_endTime;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::vector<uint32_t> m_vecThreadId;
	std::vector<std::string> m_allStock;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	uint32_t m_resultThreadId;
	BigNumber m_allFund;
	BigNumber m_allStockCount;
};