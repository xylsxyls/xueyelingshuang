#include "StockOnceRetest.h"
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

StockOnceRetest::StockOnceRetest() :
m_solutionType(SOLUTION_INIT),
m_showStockLog(false),
m_beginTime(0, 0),
m_endTime(0, 0),
m_resultThreadId(0)
{

}

StockOnceRetest::~StockOnceRetest()
{
	int32_t index = -1;
	while (index++ != m_vecThreadId.size() - 1)
	{
		CTaskThreadManager::Instance().Uninit(m_vecThreadId[index]);
	}
	CTaskThreadManager::Instance().Uninit(m_resultThreadId);
}

void StockOnceRetest::init(SolutionType solutionType,
	const std::vector<StrategyType>& vecStrategyType,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	const BigNumber initialFund,
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

	m_trade.init(m_beginTime,
		m_endTime,
		StockStrategy::instance().strategyAllStock(vecStrategyType[0], m_beginTime, m_endTime),
		m_solutionType,
		m_vecStrategyType);
}

void StockOnceRetest::load()
{
	m_trade.load();
}

void StockOnceRetest::run()
{
	if (m_beginTime.empty() || m_endTime.empty() || m_beginTime > m_endTime)
	{
		return;
	}

	BigNumber allFund = 0;

	IntDateTime calcTime = m_beginTime;
	IntDateTime calcEndDate = m_endTime - 4 * 86400;
	std::shared_ptr<StockMarket> spRunMarket = m_trade.market("600206");
	while (true)
	{
		if (spRunMarket->getMemoryDays(calcEndDate, m_endTime) == 5)
		{
			break;
		}
		calcEndDate = calcEndDate - 86400;
	}

	if (calcEndDate < m_beginTime)
	{
		return;
	}
	
	int32_t day = 0;
	do 
	{
		
		if (!spRunMarket->setDate(calcTime))
		{
			calcTime = calcTime + 86400;
			continue;
		}

		StockFund stockFund;
		stockFund.add(m_initialFund);
		IntDateTime currentTime = calcTime;
		do
		{
			std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>> sellStock;
			m_trade.sell(sellStock, currentTime, &stockFund, m_solutionType, m_vecStrategyType);

			int32_t index = -1;
			while (index++ != sellStock.size() - 1)
			{
				const std::string& stock = sellStock[index].first;
				const BigNumber& price = sellStock[index].second.first;
				const BigNumber& rate = sellStock[index].second.second;
				std::shared_ptr<StockMarket> spMarket = m_trade.market(stock);
				RCSend("maichu, date = %s, stock = %s, price = %s, rate = %s", spMarket->date().dateToString().c_str(),
					spMarket->stock().c_str(), price.toString().c_str(), rate.toString().c_str());
				stockFund.sellStock(price, rate, spMarket->day());
			}

			if (currentTime == m_endTime)
			{
				break;
			}

			std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>> buyStock;
			m_trade.buy(buyStock, currentTime, &stockFund, m_solutionType, m_vecStrategyType, calcTime);

			index = -1;
			while (index++ != buyStock.size() - 1)
			{
				const std::string& stock = buyStock[index].first;
				const BigNumber& price = buyStock[index].second.first;
				const BigNumber& rate = buyStock[index].second.second;
				std::shared_ptr<StockMarket> spMarket = m_trade.market(stock);
				RCSend("mairu, date = %s, stock = %s, price = %s, rate = %s", spMarket->date().dateToString().c_str(),
					spMarket->stock().c_str(), price.toString().c_str(), rate.toString().c_str());
				stockFund.buyStock(price, rate, spMarket->day());
			}

			//printProfit(&stockFund, currentTime);

			currentTime = currentTime + 86400;
		} while (currentTime <= m_endTime);

		if (m_showStockLog)
		{
			stockFund.printStockLog();
		}
		printProfit(&stockFund, currentTime);

		std::map<std::string, std::shared_ptr<StockDay>> dayData;
		m_trade.stockDayData(stockFund.ownedStock(), currentTime, dayData);
		allFund = allFund + stockFund.allFund(dayData);

		++day;
		calcTime = calcTime + 86400;
	} while (calcTime <= calcEndDate);

	BigNumber onceAllFund = allFund / BigNumber(day).toPrec(6).zero();
	
	RCSend("once allfund = %s, profit = %s%%", onceAllFund.toString().c_str(), ((onceAllFund - m_initialFund) / m_initialFund.toPrec(6).zero() * 100).toString().c_str());
}

void StockOnceRetest::printProfit(StockFund* stockFund, const IntDateTime& currentTime)
{
	std::map<std::string, std::shared_ptr<StockDay>> dayData;
	m_trade.stockDayData(stockFund->ownedStock(), currentTime, dayData);
	BigNumber allFund = stockFund->allFund(dayData);
	RCSend("date = %s, allFund = %s, profit = %s%%", currentTime.dateToString().c_str(),
		allFund.toString().c_str(),
		(((allFund - m_initialFund) / m_initialFund.toPrec(16).zero() * 100).toPrec(2)).toString().c_str());
}