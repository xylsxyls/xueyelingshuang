#include "ChooseStockTask.h"
#include "StockTrade/StockTradeAPI.h"
#include "StockClient.h"
#include "StockClientLogicManager.h"
#include "StockMysql/StockMysqlAPI.h"
#include "StockFund/StockFundAPI.h"
#include "ConfigManager/ConfigManagerAPI.h"
#include "StockDraw/StockDrawAPI.h"
#include "CStringManager/CStringManagerAPI.h"

ChooseStockTask::ChooseStockTask():
m_date(0, 0),
m_solutionType(SOLUTION_INIT),
m_stockFund(nullptr),
m_stockClient(nullptr)
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

	StockFund* stockFund = nullptr;
	if (m_stockFund == nullptr)
	{
		stockFund = new StockFund;
		stockFund->add(200000);
	}
	else
	{
		stockFund = m_stockFund;
	}

	std::map<std::string, std::string> stockNameMap = StockMysql::instance().stockNameMap();
	
	StockTrade stockTrade;
	stockTrade.init(m_date - 90 * 86400, m_date, m_allStock, m_solutionType, m_vecChooseParam);
	stockTrade.load();

	TradeParam tradeParam;
	tradeParam.m_stockFund = stockFund;
	stockTrade.setTradeParam(m_solutionType, tradeParam);

	RCSend("%s, strategy type = %d", m_date.dateToString().c_str(), (int32_t)m_vecChooseParam[0].m_useType);

	std::vector<std::pair<std::string, StockInfo>> sellStock;
	stockTrade.sell(sellStock, m_date, m_solutionType);
	int32_t index = sellStock.size();
	while (index-- != 0)
	{
		RCSend(("sell " + sellStock[index].first + " " + stockNameMap[sellStock[index].first] + " " + sellStock[index].second.m_rate.toPrec(6).toString()).c_str());
		LOG_SEND_ONLY_INFO(("sell " + sellStock[index].first + " " + stockNameMap[sellStock[index].first] + " " + sellStock[index].second.m_rate.toPrec(6).toString()).c_str());
	}
	m_stockClient->m_sellStock = sellStock;

	StrategyType useStrategyType = STRATEGY_INIT;
	std::vector<std::pair<std::string, StockInfo>> buyStock;
	stockTrade.buy(buyStock, m_date, m_solutionType, useStrategyType);
	index = buyStock.size();
	while (index-- != 0)
	{
		RCSend(("buy " + buyStock[index].first + " " + stockNameMap[buyStock[index].first] + " " + buyStock[index].second.m_rate.toPrec(6).toString()).c_str());
		LOG_SEND_ONLY_INFO(("buy " + buyStock[index].first + " " + stockNameMap[buyStock[index].first] + " " + buyStock[index].second.m_rate.toPrec(6).toString()).c_str());
	}
	m_stockClient->m_buyStock = buyStock;
	if (m_stockFund == nullptr)
	{
		delete stockFund;
	}
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("今日选中%1个").arg(buyStock.size()));
}

void ChooseStockTask::setParam(const IntDateTime& date,
	const std::vector<std::string>& allStock,
	const std::vector<ChooseParam>& vecChooseParam,
	SolutionType solutionType,
	StockFund* stockFund,
	StockClient* stockClient)
{
	m_date = date;
	m_stockClient = stockClient;
	m_allStock = allStock;
	m_vecChooseParam = vecChooseParam;
	m_stockFund = stockFund;
	m_solutionType = solutionType;
}