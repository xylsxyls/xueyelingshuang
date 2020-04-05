#include "StockChanceRetest.h"
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

StockChanceRetest::StockChanceRetest() :
m_solutionType(SOLUTION_INIT),
m_showStockLog(false),
m_beginTime(0, 0),
m_endTime(0, 0),
m_resultThreadId(0),
m_maxHoldDays(0),
m_terminusNum(0)
{

}

StockChanceRetest::~StockChanceRetest()
{
	int32_t index = -1;
	while (index++ != m_vecThreadId.size() - 1)
	{
		CTaskThreadManager::Instance().Uninit(m_vecThreadId[index]);
	}
	CTaskThreadManager::Instance().Uninit(m_resultThreadId);
}

void StockChanceRetest::init(SolutionType solutionType,
	const std::vector<std::pair<StrategyType, StrategyType>>& vecStrategyType,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	int32_t maxHoldDays,
	int32_t terminusNum,
	const BigNumber& initialFund,
	bool showStockLog,
	int32_t threadCount)
{
	m_solutionType = solutionType;
	m_vecStrategyType = vecStrategyType;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_maxHoldDays = maxHoldDays;
	m_terminusNum = terminusNum;
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

void StockChanceRetest::load()
{
	m_runMarket.load();
	m_trade.load();
}

void StockChanceRetest::run()
{
	if (m_beginTime.empty() || m_endTime.empty() || m_beginTime > m_endTime)
	{
		return;
	}

	std::vector<std::string> vecChanceResult;

	int32_t holdIndex = -1;
	while (holdIndex++ != m_maxHoldDays - 1)
	{
		m_runMarket.setFirstDate();

		BigNumber allFund = 0;
		BigNumber plusAllFund = 0;
		BigNumber minusAllFund = 0;

		std::vector<BigNumber> vecAllFund;

		BigNumber buyDay = 0;
		BigNumber plusDay = 0;
		while (true)
		{
			IntDateTime calcTime = m_runMarket.date();

			StockFund stockFund;
			stockFund.add(m_initialFund);

			StrategyType useStrategyType = STRATEGY_INIT;
			std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>> buyStock;
			m_trade.buy(buyStock, calcTime, &stockFund, m_solutionType, m_vecStrategyType, useStrategyType);

			int32_t index = -1;
			while (index++ != buyStock.size() - 1)
			{
				const std::string& stock = buyStock[index].first;
				const BigNumber& price = buyStock[index].second.first;
				const BigNumber& rate = buyStock[index].second.second;
				std::shared_ptr<StockMarket> spMarket = m_trade.market(stock);
				spMarket->setDate(calcTime);
				RCSend("%d天期, mairu, date = %s, stock = %s, price = %s, rate = %s",
					holdIndex,
					spMarket->date().dateToString().c_str(),
					spMarket->stock().c_str(), price.toString().c_str(), rate.toString().c_str());
				stockFund.buyStock(price, rate, spMarket->day(), useStrategyType);
			}

			StockMarket spRunMarket = m_runMarket;
			spRunMarket.setDate(calcTime);
			while (true)
			{
				if (!spRunMarket.next())
				{
					break;
				}
				IntDateTime currentTime = spRunMarket.date();
				std::vector<std::string> ownedStock = stockFund.ownedStock();
				if (ownedStock.empty())
				{
					break;
				}
				if (holdIndex == 0)
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
						spMarket->setDate(currentTime);
						RCSend("%d天期, maichu, date = %s, stock = %s, price = %s, rate = %s",
							holdIndex,
							spMarket->date().dateToString().c_str(),
							spMarket->stock().c_str(), price.toString().c_str(), rate.toString().c_str());
						stockFund.sellStock(price, rate, spMarket->day());
					}
				}
				else
				{
					int32_t index = -1;
					while (index++ != ownedStock.size() - 1)
					{
						const std::string& stock = ownedStock[index];
						std::shared_ptr<StockMarket> spMarket = m_trade.market(stock);
						IntDateTime firstBuyDate = stockFund.firstBuyDate(stock);
						if (spMarket->getMemoryDays(firstBuyDate, currentTime) < (holdIndex + 1))
						{
							continue;
						}
						spMarket->setDate(currentTime);
						BigNumber price = spMarket->day()->close();
						RCSend("%d天期, maichu, date = %s, stock = %s, price = %s, rate = 1.00",
							holdIndex,
							spMarket->date().dateToString().c_str(),
							spMarket->stock().c_str(), spMarket->day()->close().toString().c_str());
						stockFund.sellStock(price, 1, spMarket->day());
					}
				}
			};

			std::map<std::string, std::shared_ptr<StockDay>> dayData;
			m_trade.stockDayData(stockFund.ownedStock(), m_endTime, dayData);

			BigNumber currentAllFund = stockFund.allFund(dayData);
			if (currentAllFund != m_initialFund)
			{
				allFund = allFund + currentAllFund;
				vecAllFund.push_back(currentAllFund);

				if (currentAllFund < m_initialFund)
				{
					minusAllFund = minusAllFund + currentAllFund;
				}
				else
				{
					plusAllFund = plusAllFund + currentAllFund;
					++plusDay;
				}
				++buyDay;
			}
			if (!m_runMarket.next())
			{
				break;
			}
		};

		std::sort(vecAllFund.begin(), vecAllFund.end());
		std::vector<BigNumber> vecMinus;
		std::vector<BigNumber> vecPlus;
		int32_t index = -1;
		while (index++ != vecAllFund.size() - 1)
		{
			const BigNumber& sortFund = vecAllFund[index];
			if (sortFund < m_initialFund)
			{
				vecMinus.push_back(sortFund);
			}
			if ((int32_t)vecMinus.size() >= m_terminusNum)
			{
				break;
			}
		}
		index = vecAllFund.size();
		while (index-- != 0)
		{
			const BigNumber& sortFund = vecAllFund[index];
			if (sortFund > m_initialFund)
			{
				vecPlus.push_back(sortFund);
			}
			if ((int32_t)vecPlus.size() >= m_terminusNum)
			{
				break;
			}
		}

		BigNumber avgAllFund = allFund / BigNumber(buyDay).toPrec(6).zero();
		BigNumber avgMinusAllFund = minusAllFund / BigNumber(buyDay - plusDay).toPrec(6).zero();
		BigNumber avgPlusAllFund = plusAllFund / BigNumber(plusDay).toPrec(6).zero();

		vecChanceResult.push_back(CStringManager::Format("%d天期, plus: %s, minus: %s", 
			holdIndex, terminusToStr(vecPlus).c_str(), terminusToStr(vecMinus).c_str()));
		vecChanceResult.push_back(CStringManager::Format(
			"%d天期, avgPlus = %s%%, avgMinus = %s%%, avgAll = %s%%, chance = %s%%, days = %s",
			holdIndex,
			profitPercent(avgPlusAllFund).toString().c_str(),
			profitPercent(avgMinusAllFund).toString().c_str(),
			profitPercent(avgAllFund).toString().c_str(),
			(plusDay / buyDay.toPrec(6).zero() * 100).toPrec(2).toString().c_str(),
			buyDay.toString().c_str()));
	}

	int32_t index = -1;
	while (index++ != vecChanceResult.size() - 1)
	{
		if (index % 2 == 0)
		{
			RCSend("%s", vecChanceResult[index].c_str());
		}
	}
	index = -1;
	while (index++ != vecChanceResult.size() - 1)
	{
		if (index % 2 == 1)
		{
			RCSend("%s", vecChanceResult[index].c_str());
		}
	}
}

void StockChanceRetest::printProfit(StockFund* stockFund, const IntDateTime& currentTime)
{
	std::map<std::string, std::shared_ptr<StockDay>> dayData;
	m_trade.stockDayData(stockFund->ownedStock(), currentTime, dayData);
	BigNumber allFund = stockFund->allFund(dayData);
	RCSend("date = %s, allFund = %s, profit = %s%%", currentTime.dateToString().c_str(),
		allFund.toString().c_str(),
		(((allFund - m_initialFund) / m_initialFund.toPrec(16).zero() * 100).toPrec(2)).toString().c_str());
}

BigNumber StockChanceRetest::profitPercent(const BigNumber& allFund)
{
	return ((allFund - m_initialFund) / m_initialFund.toPrec(6).zero() * 100).toPrec(2);
}

std::string StockChanceRetest::terminusToStr(const std::vector<BigNumber>& vecTerminus)
{
	if (vecTerminus.empty())
	{
		return "null";
	}
	std::string result;
	int32_t index = -1;
	while (index++ != vecTerminus.size() - 2)
	{
		const BigNumber& terminus = vecTerminus[index];
		result += profitPercent(terminus).toString() + "%, ";
	}
	result += profitPercent(vecTerminus[index]).toString() + "%";
	return result;
}