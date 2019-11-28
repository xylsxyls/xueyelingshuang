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

	std::shared_ptr<StrategyInfo> spStrategyInfo;
	switch (m_strategyType)
	{
	case SAR_RISE_BACK:
	{
		SarRiseBackInfo* sarRiseBackInfo = new SarRiseBackInfo;
		sarRiseBackInfo->m_spMarket = m_spMarket;
		sarRiseBackInfo->m_spSarIndicator = m_stockSarIndicator;
		sarRiseBackInfo->m_spBollIndicator = m_stockBollIndicator;
		spStrategyInfo.reset(sarRiseBackInfo);
	}
	break;
	default:
		break;
	}

	int32_t money = 100000;

	StockFund fund;
	fund.add(money);
	do
	{
		IntDateTime date = m_spMarket->date();

		BigNumber price;
		BigNumber percent;
		BigNumber score;
		spStrategyInfo->m_fund = &fund;
		if (spStrategy->sell(date, price, percent, score, spStrategyInfo))
		{
			fund.sellStock(price, percent / BigNumber("100.0"), m_spMarket->day());
		}

		spStrategyInfo->m_fund = &fund;
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
	m_stockWrIndicator = stockWrIndicator;
	m_stockRsiIndicator = stockRsiIndicator;
	m_stockSarIndicator = stockSarIndicator;
	m_stockBollIndicator = stockBollIndicator;
	m_strategyType = strategyType;
	m_showStockLog = showStockLog;
	m_resultThreadId = resultThreadId;
	m_allFund = allFund;
	m_allStockCount = allStockCount;
}
