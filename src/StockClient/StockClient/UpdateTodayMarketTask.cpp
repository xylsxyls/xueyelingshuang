#include "UpdateTodayMarketTask.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockClient.h"
#include "StockClientLogicManager.h"
#include "CSystem/CSystemAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "SaveGroupMarketTask.h"

UpdateTodayMarketTask::UpdateTodayMarketTask():
m_stockClient(nullptr)
{

}

void UpdateTodayMarketTask::DoTask()
{
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
		if (open == "0.00" && high == "0.00" && low == "0.00" && close == "0.00")
		{
			continue;
		}
		todayMarket[0] = open;
		todayMarket[1] = high;
		todayMarket[2] = low;
		todayMarket[3] = close;
		StockMarketHelper::updateDateMarketToMysql(stock, m_stockClient->m_today, todayMarket);
	}
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("hangqing�������"));
}

void UpdateTodayMarketTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}

bool UpdateTodayMarketTask::ReExecute()
{
	return true;
}

CTask* UpdateTodayMarketTask::Clone()
{
	return new UpdateTodayMarketTask(*this);
}