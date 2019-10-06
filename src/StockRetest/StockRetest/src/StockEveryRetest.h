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
	void init(StrategyEnum strategyEnum,
		const IntDateTime& beginTime,
		const IntDateTime& endTime,
		bool showStockLog = false,
		int32_t threadCount = 0);

	void run();

private:
	StrategyEnum m_strategyEnum;
	bool m_showStockLog;
	IntDateTime m_beginTime;
	IntDateTime m_endTime;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::vector<uint32_t> m_vecThreadId;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	uint32_t m_resultThreadId;
	BigNumber m_allFund;
	BigNumber m_allStockCount;
};