#include "UpdateTodayMarketTask.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockClient.h"
#include "StockClientLogicManager.h"

UpdateTodayMarketTask::UpdateTodayMarketTask():
m_stockClient(nullptr)
{

}

void UpdateTodayMarketTask::DoTask()
{
	IntDateTime today;
	std::vector<std::string> todayMarket;
	todayMarket.push_back("");
	todayMarket.push_back("");
	todayMarket.push_back("");
	todayMarket.push_back("");
	int32_t index = -1;
	while (index++ != m_stockClient->m_todayMarket.size() - 1)
	{
		RCSend("update = %d", index + 1);
		const std::string& stock = m_stockClient->m_todayMarket[index][0];
		const std::string& open = m_stockClient->m_todayMarket[index][3];
		const std::string& high = m_stockClient->m_todayMarket[index][4];
		const std::string& low = m_stockClient->m_todayMarket[index][5];
		const std::string& close = m_stockClient->m_todayMarket[index][6];
		todayMarket[0] = open;
		todayMarket[1] = high;
		todayMarket[2] = low;
		todayMarket[3] = close;
		StockMarketHelper::updateDateMarketToMysql(stock, today, todayMarket);
	}
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("更新完成"));
}

void UpdateTodayMarketTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}