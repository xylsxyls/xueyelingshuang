#include "ChooseStockTask.h"
#include "StockTrade/StockTradeAPI.h"
#include "StockClient.h"
#include "StockClientLogicManager.h"
#include "StockMysql/StockMysqlAPI.h"

ChooseStockTask::ChooseStockTask()
{

}

void ChooseStockTask::DoTask()
{
	if (m_date.empty())
	{
		m_date = m_stockClient->m_today;
	}
	if (m_allStock.empty())
	{
		StockMysql::instance().readFilterStockFromRedis(m_date, m_allStock);
	}

	StockTrade stockTrade;
	stockTrade.init(m_date - 5 * 86400, m_date, m_allStock, SOLUTION_INIT, m_strategyType);
	stockTrade.load();
	std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>> buyStock;
	std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>> allBuyInfo;
	stockTrade.buy(buyStock, m_date, nullptr, SOLUTION_INIT, m_strategyType);
	int32_t index = buyStock.size();
	while (index-- != 0)
	{
		RCSend((buyStock[index].first + "," + buyStock[index].second.second.toString()).c_str());
		LOG_SEND_ONLY_INFO((buyStock[index].first + "," + buyStock[index].second.second.toString()).c_str());
	}
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("今日选中%1个").arg(buyStock.size()));
}

void ChooseStockTask::setParam(const IntDateTime& date,
	const std::vector<std::string>& allStock,
	StrategyType strategyType,
	StockClient* stockClient)
{
	m_date = date;
	m_stockClient = stockClient;
	m_allStock = allStock;
	m_strategyType = strategyType;
}