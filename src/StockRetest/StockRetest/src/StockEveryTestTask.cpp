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
	std::shared_ptr<Strategy> spStrategy = StockStrategy::instance().strategy(m_strategyType);

	if (m_spMarket->empty())
	{
		return;
	}

	int32_t money = 100000;

	StockFund fund;
	fund.add(money);
	std::shared_ptr<StrategyInfo> spStrategyInfo = StockStrategy::instance().strategyInfo(m_strategyType, &fund, m_spMarket, m_spIndicator);
	do
	{
		IntDateTime date = m_spMarket->date();
		BigNumber price;
		BigNumber percent;
		BigNumber score;
		if (spStrategy->sell(date, price, percent, score, spStrategyInfo))
		{
			fund.sellStock(price, percent / BigNumber("100.0"), m_spMarket->day());
		}

		if (spStrategy->buy(date, price, percent, score, spStrategyInfo))
		{
			fund.buyStock(price, percent / BigNumber("100.0"), m_spMarket->day());
		}
	} while (m_spMarket->next());
	std::map<std::string, std::shared_ptr<StockDay>> dayDate;
	dayDate[m_stock] = m_spMarket->day();
	BigNumber currentFund = fund.allFund(dayDate);
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
	StrategyType strategyType,
	bool showStockLog,
	uint32_t resultThreadId,
	BigNumber* allFund,
	BigNumber* allStockCount)
{
	m_stock = stock;
	m_spMarket = spMarket;
	m_spIndicator["wr"] = stockWrIndicator;
	m_spIndicator["rsi"] = stockRsiIndicator;
	m_spIndicator["sar"] = stockSarIndicator;
	m_spIndicator["boll"] = stockBollIndicator;
	m_strategyType = strategyType;
	m_showStockLog = showStockLog;
	m_resultThreadId = resultThreadId;
	m_allFund = allFund;
	m_allStockCount = allStockCount;
}