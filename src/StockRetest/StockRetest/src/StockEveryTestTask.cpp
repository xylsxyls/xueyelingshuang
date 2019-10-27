#include "StockEveryTestTask.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockFund/StockFundAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include "StockEveryTestResultTask.h"

StockEveryTestTask::StockEveryTestTask():
m_spMarket(nullptr),
m_allFund(nullptr),
m_allStockCount(nullptr),
m_showStockLog(false)
{

}

void StockEveryTestTask::DoTask()
{
	m_spMarket->load();
	std::shared_ptr<StockMarket> spMarket(new StockMarket(*m_spMarket));
	std::shared_ptr<Strategy> spStrategy = StockStrategy::instance().strategy(m_stock,
		spMarket,
		m_stockWrIndicator,
		m_stockRsiIndicator,
		m_stockSarIndicator,
		m_stockBollIndicator,
		m_strategyEnum);
	
	spStrategy->load();

	if (m_spMarket->empty())
	{
		return;
	}

	int32_t money = 100000;

	StockFund fund;
	fund.add(money);
	do
	{
		IntDateTime date = m_spMarket->date();

		BigNumber price;
		BigNumber percent;
		if (spStrategy->sell(date, price, percent))
		{
			fund.sellStock(price, percent / BigNumber("100.0"), m_spMarket->day());
		}

		if (spStrategy->buy(date, price, percent))
		{
			fund.buyStock(price, percent / BigNumber("100.0"), m_spMarket->day());
		}
	} while (m_spMarket->next());
	BigNumber currentFund = fund.allFund(m_spMarket->day());
	if (currentFund == money)
	{
		return;
	}

	std::vector<std::string> stockLog;
	if (m_showStockLog)
	{
		stockLog = fund.stockLog();
	}

	std::shared_ptr<StockEveryTestResultTask> spStockEveryTestResultTask(new StockEveryTestResultTask);
	spStockEveryTestResultTask->setParam(m_stock, stockLog, currentFund, m_allFund, m_allStockCount);
	CTaskThreadManager::Instance().GetThreadInterface(m_resultThreadId)->PostTask(spStockEveryTestResultTask);
}

void StockEveryTestTask::setParam(const std::string& stock,
	const std::shared_ptr<StockMarket>& spMarket,
	const std::shared_ptr<StockWrIndicator>& stockWrIndicator,
	const std::shared_ptr<StockRsiIndicator>& stockRsiIndicator,
	const std::shared_ptr<StockSarIndicator>& stockSarIndicator,
	const std::shared_ptr<StockBollIndicator>& stockBollIndicator,
	StrategyEnum strategyEnum,
	bool showStockLog,
	uint32_t resultThreadId,
	BigNumber* allFund,
	BigNumber* allStockCount)
{
	m_stock = stock;
	m_spMarket = spMarket;
	m_stockWrIndicator = stockWrIndicator;
	m_stockRsiIndicator = stockRsiIndicator;
	m_stockSarIndicator = stockSarIndicator;
	m_stockBollIndicator = stockBollIndicator;
	m_strategyEnum = strategyEnum;
	m_showStockLog = showStockLog;
	m_resultThreadId = resultThreadId;
	m_allFund = allFund;
	m_allStockCount = allStockCount;
}
