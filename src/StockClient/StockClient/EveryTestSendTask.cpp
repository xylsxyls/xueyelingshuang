#include "EveryTestSendTask.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockMysql/StockMysqlAPI.h"
#include "EveryTestTask.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockClient.h"
#include "StockClientLogicManager.h"

EveryTestSendTask::EveryTestSendTask():
m_stockClient(nullptr)
{

}

void EveryTestSendTask::DoTask()
{
	IntDateTime beginTime = "2010-01-01";
	IntDateTime endTime = "2019-04-01";

	std::vector<std::string> vecStock = StockMysql::instance().allStock();
	std::sort(vecStock.begin(), vecStock.end());
	m_stockClient->m_stockCount = vecStock.size();
	int32_t index = -1;
	while (index++ != vecStock.size() - 1)
	{
		std::shared_ptr<EveryTestTask> spEveryTestTask(new EveryTestTask);
		std::shared_ptr<StockMarket> spMarket(new StockMarket);
		spMarket->loadFromRedis(vecStock[index], beginTime, endTime);
		StockIndicator::instance().loadIndicatorFromRedis(vecStock[index], beginTime, endTime);
		std::shared_ptr<StockWrIndicator> spStockWrIndicator = StockIndicator::instance().wr();
		std::shared_ptr<StockRsiIndicator> spStockRsiIndicator = StockIndicator::instance().rsi();
		std::shared_ptr<StockSarIndicator> spStockSarIndicator = StockIndicator::instance().sar();
		std::shared_ptr<StockBollIndicator> spStockBollIndicator = StockIndicator::instance().boll();
		spEveryTestTask->setParam(vecStock[index],
			spMarket,
			spStockWrIndicator,
			spStockRsiIndicator,
			spStockSarIndicator,
			spStockBollIndicator,
			m_stockClient);
		CTaskThreadManager::Instance().GetThreadInterface(m_stockClient->m_threadId[index % m_stockClient->m_threadCount])->PostTask(spEveryTestTask);
	}
}
void EveryTestSendTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}