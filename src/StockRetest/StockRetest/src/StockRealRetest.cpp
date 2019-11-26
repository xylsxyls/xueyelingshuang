#include "StockRealRetest.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "StockEveryTestTask.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include "StockMysql/StockMysqlAPI.h"
#include <algorithm>
#include "StockTrade/StockTradeAPI.h"
#include "StockFund/StockFundAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include <math.h>

StockRealRetest::StockRealRetest():
m_solutionType(SOLUTION_INIT),
m_strategyType(STRATEGY_INIT),
m_showStockLog(false),
m_beginTime(0, 0),
m_endTime(0, 0),
m_resultThreadId(0)
{
	
}

StockRealRetest::~StockRealRetest()
{
	int32_t index = -1;
	while (index++ != m_vecThreadId.size() - 1)
	{
		CTaskThreadManager::Instance().Uninit(m_vecThreadId[index]);
	}
	CTaskThreadManager::Instance().Uninit(m_resultThreadId);
}

void StockRealRetest::init(SolutionType solutionType,
	StrategyType strategyType,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	const BigNumber initialFund,
	bool showStockLog,
	int32_t threadCount)
{
	m_solutionType = solutionType;
	m_strategyType = strategyType;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_initialFund = initialFund;
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

	m_fund.add(m_initialFund);

	m_trade.init(m_beginTime,
		m_endTime,
		StockStrategy::instance().strategyAllStock(m_strategyType, m_beginTime, m_endTime),
		m_solutionType,
		m_strategyType);
}

void StockRealRetest::load()
{
	m_trade.load();
}

void StockRealRetest::run()
{
	if (m_beginTime.empty() || m_endTime.empty() || m_beginTime > m_endTime)
	{
		return;
	}

	IntDateTime currentTime = m_beginTime;
	do
	{
		std::vector<std::string> vecOwnStock = m_fund.ownedStock();
		BigNumber price;
		BigNumber rate;
		int32_t index = -1;
		while (index++ != vecOwnStock.size() - 1)
		{
			const std::string& stock = vecOwnStock[index];
			std::shared_ptr<StockMarket> spMarket = m_trade.market(stock);
			if (!spMarket->setDate(currentTime))
			{
				continue;
			}
			if (!m_trade.sell(stock, currentTime, price, rate, &m_fund, m_solutionType, m_strategyType))
			{
				continue;
			}
			m_fund.sellStock(price, rate, spMarket->day());
		}
		
		if (currentTime == m_endTime)
		{
			break;
		}
		std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>> buyStock;
		m_trade.buy(buyStock, currentTime, &m_fund, m_solutionType, m_strategyType);

		index = -1;
		while (index++ != buyStock.size() - 1)
		{
			const std::string& stock = buyStock[index].first;
			const BigNumber& price = buyStock[index].second.first;
			const BigNumber& rate = buyStock[index].second.second;
			std::shared_ptr<StockMarket> spMarket = m_trade.market(stock);
			if (!spMarket->setDate(currentTime))
			{
				continue;
			}
			m_fund.buyStock(price, rate, spMarket->day());
		}

		printProfit(currentTime);

		currentTime = currentTime + 86400;
	} while (currentTime <= m_endTime);
	
	if (m_showStockLog)
	{
		m_fund.printStockLog();
	}
	printProfit(currentTime);
}

void StockRealRetest::printProfit(const IntDateTime& currentTime)
{
	std::map<std::string, std::shared_ptr<StockDay>> dayData;
	m_trade.stockDayData(m_fund.ownedStock(), currentTime, dayData);
	BigNumber allFund = m_fund.allFund(dayData);
	RCSend("date = %s, allFund = %s, profit = %s%%", currentTime.dateToString().c_str(),
		allFund.toString().c_str(),
		(((allFund - m_initialFund) / m_initialFund.toPrec(16).zero() * 100).toPrec(2)).toString().c_str());
}