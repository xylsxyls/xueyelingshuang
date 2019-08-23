#include "SaveMarketToMysqlTask.h"
#include "StockClientLogicManager.h"
#include "CSystem/CSystemAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockMysql/StockMysqlAPI.h"

SaveMarketToMysqlTask::SaveMarketToMysqlTask():
m_stockClient(nullptr)
{

}

void SaveMarketToMysqlTask::DoTask()
{
	std::string marketDir = CSystem::GetCurrentExePath() + "CurrentAllMarketFile\\";
	std::vector<std::string> allStock = StockMysql::instance().allStockFromMysql();

	StockMysql::instance().deleteMarketDb();
	StockMysql::instance().createMarketDb();
	int32_t index = -1;
	while (index++ != allStock.size() - 1)
	{
		RCSend("saveMarket = %d", index + 1);
		const std::string& stock = allStock[index];
		std::string marketFilePath = marketDir + stock + ".txt";
		StockMarketHelper::saveMarketToMysql(stock, marketFilePath);
	}

	emit StockClientLogicManager::instance().taskTip(QStringLiteral("所有hangqing已入库"));
}

void SaveMarketToMysqlTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}