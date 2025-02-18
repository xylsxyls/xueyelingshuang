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
	const std::vector<ChooseParam>& vecChooseParam,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	const BigNumber& initialFund,
	bool showStockLog,
	int32_t threadCount)
{
	m_solutionType = solutionType;
	m_vecChooseParam = vecChooseParam;
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

	m_trade.init(beginTime,
		endTime,
		m_solutionType,
		m_vecChooseParam);

	m_runMarket = *m_trade.runMarket();
	m_runMarket.setNewDate(beginTime);
	m_beginTime = m_runMarket.date();
	m_runMarket.setLastDate(endTime);
	m_endTime = m_runMarket.date();
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
	m_runMarket.setDate(m_endTime);
	int32_t count = 4;
	while (count-- != 0)
	{
		m_runMarket.previous();
	}

	IntDateTime calcEndDate = m_runMarket.date();

	if (calcEndDate <= m_beginTime)
	{
		return;
	}

	m_runMarket.setDate(m_beginTime);
	StockMarket runMarket = m_runMarket;
	
	int32_t day = 0;
	do
	{
		StockFund stockFund;
		stockFund.add(m_initialFund);

		TradeParam tradeParam;
		tradeParam.m_stockFund = &stockFund;
		tradeParam.m_onceDate = calcTime;
		m_trade.setTradeParam(m_solutionType, tradeParam);
		m_trade.setTradeParam(DISPOSABLE_STRATEGY, tradeParam);

		IntDateTime currentTime = calcTime;
		runMarket.setDate(currentTime);
		do
		{
			std::vector<std::pair<std::string, StockInfo>> sellStock;
			m_trade.sell(sellStock, currentTime, m_solutionType);

			int32_t index = -1;
			while (index++ != sellStock.size() - 1)
			{
				const std::string& stock = sellStock[index].first;
				const BigNumber& price = sellStock[index].second.m_price;
				const BigNumber& rate = sellStock[index].second.m_rate;
				std::shared_ptr<StockMarket> spMarket = m_trade.market(stock);
				spMarket->setDate(currentTime);
				RCSend("maichu, date = %s, stock = %s, price = %s, rate = %s", spMarket->date().dateToString().c_str(),
					spMarket->stock().c_str(), price.toString().c_str(), rate.toString().c_str());
				stockFund.sellStock(price, rate, spMarket->day());
			}

			if (currentTime == m_endTime)
			{
				break;
			}

			std::shared_ptr<ChooseParam> useChooseParam;
			std::vector<std::pair<std::string, StockInfo>> buyStock;
			m_trade.buy(buyStock, currentTime, m_solutionType, useChooseParam);

			index = -1;
			while (index++ != buyStock.size() - 1)
			{
				const std::string& stock = buyStock[index].first;
				const BigNumber& price = buyStock[index].second.m_price;
				const BigNumber& rate = buyStock[index].second.m_rate;
				std::shared_ptr<StockMarket> spMarket = m_trade.market(stock);
				spMarket->setDate(currentTime);
				RCSend("mairu, date = %s, stock = %s, price = %s, rate = %s", spMarket->date().dateToString().c_str(),
					spMarket->stock().c_str(), price.toString().c_str(), rate.toString().c_str());
				stockFund.buyStock(price, rate, spMarket->day(), useChooseParam);
			}

			//printProfit(&stockFund, currentTime);

			runMarket.next();
			currentTime = runMarket.date();
		} while (currentTime <= m_endTime);

		if (m_showStockLog)
		{
			stockFund.printStockLog();
		}
		printProfit(&stockFund, currentTime);

		std::map<std::string, std::shared_ptr<StockDay>> dayData;
		m_trade.stockDayData(stockFund.ownedStock(), currentTime, dayData);

		BigNumber currentAllFund = stockFund.allFund(dayData);
		if (currentAllFund != m_initialFund)
		{
			allFund = allFund + currentAllFund;
			++day;
		}
		if (!m_runMarket.next())
		{
			break;
		}
		calcTime = m_runMarket.date();
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