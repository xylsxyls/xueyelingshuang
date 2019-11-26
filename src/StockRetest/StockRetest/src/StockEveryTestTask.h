#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "StockStrategy/StockStrategyAPI.h"

class Strategy;
class StockMarket;
class StockWrIndicator;
class StockRsiIndicator;
class StockSarIndicator;
class StockBollIndicator;
class BigNumber;

class StockEveryTestTask : public CTask
{
public:
	StockEveryTestTask();

public:
	void DoTask();

	void setParam(const std::string& stock,
		const std::shared_ptr<StockMarket>& spMarket,
		const std::shared_ptr<StockWrIndicator>& stockWrIndicator,
		const std::shared_ptr<StockRsiIndicator>& stockRsiIndicator,
		const std::shared_ptr<StockSarIndicator>& stockSarIndicator,
		const std::shared_ptr<StockBollIndicator>& stockBollIndicator,
		StrategyType strategyType,
		bool showStockLog,
		uint32_t resultThreadId,
		BigNumber* allFund,
		BigNumber* allStockCount);

private:
	std::string m_stock;
	std::shared_ptr<StockMarket> m_spMarket;
	std::shared_ptr<StockWrIndicator> m_stockWrIndicator;
	std::shared_ptr<StockRsiIndicator> m_stockRsiIndicator;
	std::shared_ptr<StockSarIndicator> m_stockSarIndicator;
	std::shared_ptr<StockBollIndicator> m_stockBollIndicator;
	StrategyType m_strategyType;
	bool m_showStockLog;
	uint32_t m_resultThreadId;
	BigNumber* m_allFund;
	BigNumber* m_allStockCount;
};