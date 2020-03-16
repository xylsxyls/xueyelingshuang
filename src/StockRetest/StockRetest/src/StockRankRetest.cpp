#include "StockRankRetest.h"
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

StockRankRetest::StockRankRetest():
m_solutionType(SOLUTION_INIT),
m_showStockLog(false),
m_beginTime(0, 0),
m_endTime(0, 0),
m_resultThreadId(0)
{
	
}

StockRankRetest::~StockRankRetest()
{
	int32_t index = -1;
	while (index++ != m_vecThreadId.size() - 1)
	{
		CTaskThreadManager::Instance().Uninit(m_vecThreadId[index]);
	}
	CTaskThreadManager::Instance().Uninit(m_resultThreadId);
}

void StockRankRetest::init(SolutionType solutionType,
	const std::vector<std::pair<StrategyType, StrategyType>>& vecStrategyType,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	const BigNumber& initialFund,
	bool showStockLog,
	int32_t threadCount)
{
	m_solutionType = solutionType;
	m_vecStrategyType = vecStrategyType;
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

	std::set<StrategyType> setType;
	int32_t index = -1;
	while (index++ != m_vecStrategyType.size() - 1)
	{
		setType.insert(m_vecStrategyType[index].first);
		setType.insert(m_vecStrategyType[index].second);
	}
	std::vector<StrategyType> vecInitStrategyType;
	for (auto itSetType = setType.begin(); itSetType != setType.end(); ++itSetType)
	{
		const StrategyType& strategyType = *itSetType;
		vecInitStrategyType.push_back(strategyType);
	}

	m_trade.init(m_beginTime,
		m_endTime,
		StockStrategy::instance().strategyAllStock(m_beginTime, m_endTime),
		m_solutionType,
		vecInitStrategyType);
}

void StockRankRetest::load()
{
	m_trade.load();
}

void StockRankRetest::run()
{
	if (m_beginTime.empty() || m_endTime.empty() || m_beginTime > m_endTime)
	{
		return;
	}

	IntDateTime currentTime = m_beginTime;
	do
	{
		std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>> sellStock;
		m_trade.sell(sellStock, currentTime, &m_fund, m_solutionType, m_vecStrategyType);

		int32_t index = -1;
		while (index++ != sellStock.size() - 1)
		{
			const std::string& stock = sellStock[index].first;
			const BigNumber& price = sellStock[index].second.first;
			const BigNumber& rate = sellStock[index].second.second;
			std::shared_ptr<StockMarket> spMarket = m_trade.market(stock);
			RCSend("maichu, date = %s, stock = %s, price = %s, rate = %s", spMarket->date().dateToString().c_str(),
				spMarket->stock().c_str(), price.toString().c_str(), rate.toString().c_str());
			m_fund.sellStock(price, rate, spMarket->day());
		}

		if (currentTime == m_endTime)
		{
			break;
		}

		std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>> buyStock;
		StrategyType useStrategyType = STRATEGY_INIT;
		m_trade.buy(buyStock, currentTime, &m_fund, m_solutionType, m_vecStrategyType, useStrategyType);

		index = -1;
		while (index++ != buyStock.size() - 1)
		{
			const std::string& stock = buyStock[index].first;
			const BigNumber& price = buyStock[index].second.first;
			const BigNumber& rate = buyStock[index].second.second;
			std::shared_ptr<StockMarket> spMarket = m_trade.market(stock);
			RCSend("mairu, date = %s, stock = %s, price = %s, rate = %s", spMarket->date().dateToString().c_str(),
				spMarket->stock().c_str(), price.toString().c_str(), rate.toString().c_str());
			m_fund.buyStock(price, rate, spMarket->day(), useStrategyType);
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

void StockRankRetest::printProfit(const IntDateTime& currentTime)
{
	std::map<std::string, std::shared_ptr<StockDay>> dayData;
	m_trade.stockDayData(m_fund.ownedStock(), currentTime, dayData);
	BigNumber allFund = m_fund.allFund(dayData);
	RCSend("date = %s, allFund = %s, profit = %s%%", currentTime.dateToString().c_str(),
		allFund.toString().c_str(),
		(((allFund - m_initialFund) / m_initialFund.toPrec(16).zero() * 100).toPrec(2)).toString().c_str());
}