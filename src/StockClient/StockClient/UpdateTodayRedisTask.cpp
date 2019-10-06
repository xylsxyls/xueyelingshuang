#include "UpdateTodayRedisTask.h"
#include "StockClient.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockMysql/StockMysqlAPI.h"
#include "StockClientLogicManager.h"

UpdateTodayRedisTask::UpdateTodayRedisTask()
{

}

void UpdateTodayRedisTask::DoTask()
{
	std::vector<std::string> todayMarket;
	todayMarket.push_back("");
	todayMarket.push_back("");
	todayMarket.push_back("");
	todayMarket.push_back("");
	int32_t index = -1;
	while (index++ != m_stockClient->m_todayMarket.size() - 1)
	{
		RCSend("update redis = %d", index + 1);
		const std::string& stock = m_stockClient->m_todayMarket[index][0];

		auto itUpdateStock = std::find(m_allUpdateStock.begin(), m_allUpdateStock.end(), stock);
		if (itUpdateStock == m_allUpdateStock.end())
		{
			continue;
		}

		const std::string& open = m_stockClient->m_todayMarket[index][3];
		const std::string& high = m_stockClient->m_todayMarket[index][4];
		const std::string& low = m_stockClient->m_todayMarket[index][5];
		const std::string& close = m_stockClient->m_todayMarket[index][6];
		if (open == "0.00" && high == "0.00" && low == "0.00" && close == "0.00")
		{
			continue;
		}
		todayMarket[0] = open;
		todayMarket[1] = high;
		todayMarket[2] = low;
		todayMarket[3] = close;
		StockMarketHelper::updateDateMarketToRedis(stock, m_stockClient->m_today, todayMarket);
	}
	StockIndicator::instance().updateDateIndicatorToRedis(m_stockClient->m_today);
	StockMysql::instance().updateAllDataRedis(m_stockClient->m_today, m_allUpdateStock);
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("redis更新完成"));
}

void UpdateTodayRedisTask::setParam(const std::vector<std::string>& allUpdateStock, StockClient* stockClient)
{
	m_allUpdateStock = allUpdateStock;
	m_stockClient = stockClient;
}