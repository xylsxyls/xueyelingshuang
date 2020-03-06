#include "StockEveryRetest.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "StockEveryTestTask.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include "StockMysql/StockMysqlAPI.h"
#include <algorithm>

StockEveryRetest::StockEveryRetest():
m_strategyType(STRATEGY_INIT),
m_showStockLog(false),
m_beginTime(0, 0),
m_endTime(0, 0),
m_resultThreadId(0)
{
	
}

StockEveryRetest::~StockEveryRetest()
{
	int32_t index = -1;
	while (index++ != m_vecThreadId.size() - 1)
	{
		CTaskThreadManager::Instance().Uninit(m_vecThreadId[index]);
	}
	CTaskThreadManager::Instance().Uninit(m_resultThreadId);
}

void StockEveryRetest::init(StrategyType strategyType,
	const std::vector<std::string>& allStock,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	bool showStockLog,
	int32_t threadCount)
{
	m_strategyType = strategyType;
	m_allStock = allStock;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_showStockLog = showStockLog;
	if (threadCount == 0)
	{
		threadCount = CSystem::GetCPUCoreCount();
	}
	while (threadCount-- != 0)
	{
		m_vecThreadId.push_back(CTaskThreadManager::Instance().Init());
	}
	m_resultThreadId = CTaskThreadManager::Instance().Init();
}

void StockEveryRetest::run()
{
	std::vector<std::string> vecStock = m_allStock.empty() ? StockStrategy::instance().strategyAllStock(m_beginTime, m_endTime) : m_allStock;
	std::sort(vecStock.begin(), vecStock.end());
	int32_t threadCount = m_vecThreadId.size();
	int32_t index = -1;
	while (index++ != vecStock.size() - 1)
	{
		const std::string& stock = vecStock[index];
		std::shared_ptr<StockEveryTestTask> spStockEveryTestTask(new StockEveryTestTask);
		std::shared_ptr<StockMarket> spMarket(new StockMarket);
		spMarket->loadFromRedis(stock, m_beginTime, m_endTime);
		StockIndicator::instance().loadIndicatorFromRedis(stock, m_beginTime, m_endTime);
		std::shared_ptr<StockWrIndicator> spStockWrIndicator = StockIndicator::instance().wr();
		std::shared_ptr<StockRsiIndicator> spStockRsiIndicator = StockIndicator::instance().rsi();
		std::shared_ptr<StockSarIndicator> spStockSarIndicator = StockIndicator::instance().sar();
		std::shared_ptr<StockBollIndicator> spStockBollIndicator = StockIndicator::instance().boll();
		spStockEveryTestTask->setParam(stock,
			spMarket,
			spStockWrIndicator,
			spStockRsiIndicator,
			spStockSarIndicator,
			spStockBollIndicator,
			m_strategyType,
			m_showStockLog,
			m_resultThreadId,
			&m_allFund,
			&m_allStockCount);
		CTaskThreadManager::Instance().GetThreadInterface(m_vecThreadId[index % threadCount])->PostTask(spStockEveryTestTask);
	}
	index = -1;
	while (index++ != m_vecThreadId.size() - 1)
	{
		CTaskThreadManager::Instance().WaitForEnd(m_vecThreadId[index]);
	}
	CTaskThreadManager::Instance().WaitForEnd(m_resultThreadId);
	if (m_allStockCount == 0)
	{
		RCSend("error allStockCount = 0");
		return;
	}
	RCSend("avg = %s, count = %s", (m_allFund / m_allStockCount.toPrec(6)).toPrec(2).toString().c_str(), m_allStockCount.toString().c_str());
}