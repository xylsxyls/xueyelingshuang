#include "StockDaysRetest.h"
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

StockDaysRetest::StockDaysRetest():
m_solutionType(SOLUTION_INIT),
m_showStockLog(false),
m_beginTime(0, 0),
m_endTime(0, 0),
m_resultThreadId(0)
{
	
}

StockDaysRetest::~StockDaysRetest()
{
	int32_t index = -1;
	while (index++ != m_vecThreadId.size() - 1)
	{
		CTaskThreadManager::Instance().Uninit(m_vecThreadId[index]);
	}
	CTaskThreadManager::Instance().Uninit(m_resultThreadId);
}

void StockDaysRetest::init(SolutionType solutionType,
	const std::vector<ChooseParam>& vecChooseParam,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	const BigNumber& initialFund,
	bool showStockLog,
	int32_t threadCount)
{
	m_solutionType = solutionType;
	m_vecChooseParam = vecChooseParam;
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

	m_runMarket.loadFromRedis("000001", m_beginTime, m_endTime);

	m_trade.init(m_beginTime,
		m_endTime,
		StockStrategy::instance().strategyAllStock(m_beginTime, m_endTime),
		m_solutionType,
		m_vecChooseParam);
}

void StockDaysRetest::load()
{
	m_trade.load();
	m_runMarket.load();
}

void StockDaysRetest::run()
{
	if (m_beginTime.empty() || m_endTime.empty() || m_beginTime > m_endTime)
	{
		return;
	}

	m_runMarket.setFirstDate();

	TradeParam tradeParam;
	tradeParam.m_stockFund = &m_fund;
	m_trade.setTradeParam(m_solutionType, tradeParam);

	BigNumber hasDays = 0;
	BigNumber days = 0;
	BigNumber buyDays = 0;
	BigNumber hasPercent = 0;

	while (true)
	{
		IntDateTime currentTime = m_runMarket.date();
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
			m_fund.sellStock(price, rate, spMarket->day());
		}

		if (currentTime >= m_endTime)
		{
			break;
		}

		std::vector<std::pair<std::string, StockInfo>> buyStock;
		StrategyType useStrategyType = STRATEGY_INIT;
		bool isBuy = m_trade.buy(buyStock, currentTime, m_solutionType, useStrategyType);
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
			m_fund.buyStock(price, rate, spMarket->day(), useStrategyType);
		}

		m_runMarket.next();
		++days;
		if (isBuy)
		{
			++buyDays;
		}

		std::vector<std::string> ownedStock = m_fund.ownedStock();
		if (ownedStock.empty())
		{
			continue;
		}

		++hasDays;

		std::map<std::string, std::shared_ptr<StockDay>> dayData;
		m_trade.stockDayData(ownedStock, currentTime, dayData);
		BigNumber currentAllFund = m_fund.allFund(dayData);
		BigNumber currentAvailableFund = m_fund.availableFund();

		hasPercent = hasPercent + ((currentAllFund - currentAvailableFund) / currentAllFund.toPrec(6));
	}
	
	if (m_showStockLog)
	{
		m_fund.printStockLog();
	}

	RCSend("day = %s%%, buy day = %s%%, day percent = %s%%", (hasDays / days.toPrec(6).zero() * 100).toPrec(2).toString().c_str(),
		(buyDays / days.toPrec(6).zero() * 100).toPrec(2).toString().c_str(),
		(hasPercent / hasDays.toPrec(6).zero() * 100).toPrec(2).toString().c_str());
}

void StockDaysRetest::printProfit(const IntDateTime& currentTime)
{
	std::map<std::string, std::shared_ptr<StockDay>> dayData;
	m_trade.stockDayData(m_fund.ownedStock(), currentTime, dayData);
	BigNumber allFund = m_fund.allFund(dayData);
	RCSend("date = %s, allFund = %s, profit = %s%%", currentTime.dateToString().c_str(),
		allFund.toString().c_str(),
		(((allFund - m_initialFund) / m_initialFund.toPrec(16).zero() * 100).toPrec(2)).toString().c_str());
}