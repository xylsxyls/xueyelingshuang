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
	
	StockTrade stockTrade;
	stockTrade.init(m_date - 5 * 86400, m_date, m_allStock, m_solutionType, m_vecStrategyType);
	stockTrade.load();

	std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>> sellStock;
	stockTrade.sell(sellStock, m_date, stockFund, m_solutionType, m_vecStrategyType);
	int32_t index = sellStock.size();
	while (index-- != 0)
	{
		RCSend((sellStock[index].first + "," + sellStock[index].second.second.toString()).c_str());
		LOG_SEND_ONLY_INFO((sellStock[index].first + "," + sellStock[index].second.second.toString()).c_str());
	}
	m_stockClient->m_sellStock = sellStock;

	std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>> buyStock;
	std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>> allBuyInfo;
	stockTrade.buy(buyStock, m_date, stockFund, m_solutionType, m_vecStrategyType);
	index = buyStock.size();
	while (index-- != 0)
	{
		RCSend((buyStock[index].first + "," + buyStock[index].second.second.toString()).c_str());
		LOG_SEND_ONLY_INFO((buyStock[index].first + "," + buyStock[index].second.second.toString()).c_str());
	}
	m_stockClient->m_buyStock = buyStock;
	if (m_stockFund == nullptr)
	{
		delete stockFund;
	}
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("����ѡ��%1��").arg(buyStock.size()));
}

void ChooseStockTask::setParam(const IntDateTime& date,
	const std::vector<std::string>& allStock,
	const std::vector<StrategyType>& vecStrategyType,
	SolutionType solutionType,
	StockFund* stockFund,
	StockClient* stockClient)
{
	m_date = date;
	m_stockClient = stockClient;
	m_allStock = allStock;
	m_vecStrategyType = vecStrategyType;
	m_stockFund = stockFund;
	m_solutionType = solutionType;
}