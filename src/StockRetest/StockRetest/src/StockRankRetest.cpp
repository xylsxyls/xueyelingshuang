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
#include "CStringManager/CStringManagerAPI.h"

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

	m_runMarket.loadFromRedis("000001", m_beginTime, m_endTime);

	m_trade.init(m_beginTime,
		m_endTime,
		StockStrategy::instance().strategyAllStock(m_beginTime, m_endTime),
		m_solutionType,
		vecInitStrategyType);
}

void StockRankRetest::load()
{
	m_trade.load();
	m_runMarket.load();
}

void StockRankRetest::run()
{
	if (m_beginTime.empty() || m_endTime.empty() || m_beginTime > m_endTime)
	{
		return;
	}

	if (!m_runMarket.setDate(m_beginTime))
	{
		return;
	}
	IntDateTime calcTime = m_runMarket.date();
	
	while (calcTime < m_endTime)
	{
		StockFund stockFund;
		stockFund.add(m_initialFund);

		StrategyType useStrategyType = STRATEGY_INIT;
		std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>> buyStock;
		if (!m_trade.buy(buyStock, calcTime, &stockFund, m_solutionType, m_vecStrategyType, useStrategyType))
		{
			if (!m_runMarket.next())
			{
				break;
			}
			calcTime = m_runMarket.date();
			continue;
		}
		RCSend("date = %s", calcTime.dateToString().c_str());

		std::vector<StockFund> vecStockFund;
		std::vector<std::string> vecStockRank;

		int32_t index = -1;
		while (index++ != buyStock.size() - 1)
		{
			vecStockFund.push_back(StockFund());
			vecStockFund.back().add(m_initialFund);
			const std::string& stock = buyStock[index].first;
			const BigNumber& price = buyStock[index].second.first;
			std::shared_ptr<StockMarket> spMarket = m_trade.market(stock);
			spMarket->setDate(calcTime);
			vecStockFund.back().buyStock(price, 1, spMarket->day(), useStrategyType);
			spMarket->previous();
			std::string preAvg = spMarket->day()->fourAvgChgValue().toString();
			spMarket->next();
			std::string avg = spMarket->day()->fourAvgChgValue().toString();
			vecStockRank.push_back(stock + " " + preAvg + "%, " + avg + "%");
		}

		IntDateTime currentTime = calcTime;
		int32_t dayIndex = -1;
		while (dayIndex++ != 10 - 1)
		{
			m_runMarket.setDate(currentTime);
			if (!m_runMarket.next())
			{
				break;
			}
			currentTime = m_runMarket.date();

			int32_t fundIndex = -1;
			while (fundIndex++ != vecStockFund.size() - 1)
			{
				BigNumber allChg = 0;
				StockFund& fund = vecStockFund[fundIndex];
				std::string stock = fund.ownedStock()[0];
				std::shared_ptr<StockMarket> spMarket = m_trade.market(stock);
				spMarket->setDate(currentTime);
				std::shared_ptr<StockDay> spDay = spMarket->day();
				fund.stockChg(stock, spDay, allChg);
				BigNumber dayChg = spDay->chgValue();
				vecStockRank[fundIndex].append(CStringManager::Format(" (%d) %s%%,%s%%", dayIndex + 1, dayChg.toString().c_str(), allChg.toString().c_str()));
			}
		}

		index = -1;
		while (index++ != buyStock.size() - 1)
		{
			RCSend("%s", vecStockRank[index].c_str());
		}
		
		m_runMarket.setDate(calcTime);
		if (!m_runMarket.next())
		{
			break;
		}
		calcTime = m_runMarket.date();
	}
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