#include "AvgFundHighScore.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>
#include "AvgFundHighScoreAllInfo.h"
#include "AvgFundHighScoreInfo.h"
#include "StockFund/StockFundAPI.h"
#include "StockMarket/StockMarketAPI.h"

bool sortFun(const std::pair<std::string, std::pair<BigNumber, BigNumber>>& stock1,
	const std::pair<std::string, std::pair<BigNumber, BigNumber>>& stock2)
{
	return stock1.second.second > stock2.second.second;
}

AvgFundHighScore::AvgFundHighScore():
m_stockNum(0),
m_minPollSize(0),
m_sellNum(0)
{
	m_solutionType = AVG_FUND_HIGH_SCORE;
}

void AvgFundHighScore::init(int32_t stockNum, int32_t minPollSize)
{
	m_stockNum = stockNum;
	m_minPollSize = minPollSize;
}

bool AvgFundHighScore::buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionAllInfo>& solutionAllInfo)
{
	if (!strategyBuy(buyStock, date, solutionAllInfo))
	{
		return false;
	}

	int32_t index = -1;
	while (index++ != buyStock.size() - 1)
	{
		const std::string& stock = buyStock[index].first;
		RCSend("date = %s, stock = %s", date.dateToString().c_str(), stock.c_str());
	}

	int32_t pollsize = m_spStrategy->buyPollSize();
	RCSend("pollsize = %d", pollsize);

	if (buyStock.size() < m_minPollSize)
	{
		buyStock.clear();
		return false;
	}
	
	while (buyStock.size() > m_stockNum - solutionAllInfo->m_fund->allBuyInfo()->size())
	{
		buyStock.erase(--buyStock.end());
	}

	BigNumber allScore = 0;
	index = -1;
	while (index++ != buyStock.size() - 1)
	{
		allScore = allScore + buyStock[index].second.second;
	}
	index = -1;
	while (index++ != buyStock.size() - 1)
	{
		BigNumber score = buyStock[index].second.second;
		buyStock[index].second.second = score / BigNumber((std::max)(atoi(allScore.toString().c_str()), m_stockNum * 100)).toPrec(2).zero();
		allScore = allScore - score;
	}
	m_sellNum = 0;
	return true;
}

bool AvgFundHighScore::sell(const IntDateTime& date,
	BigNumber& price,
	BigNumber& rate,
	const std::shared_ptr<SolutionInfo>& solutionInfo,
	const std::shared_ptr<SolutionAllInfo>& solutionAllInfo)
{
	const std::shared_ptr<AvgFundHighScoreInfo>& avgFundHighScoreInfo = std::dynamic_pointer_cast<AvgFundHighScoreInfo>(solutionInfo);
	
	std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>> buyStock;
	strategyBuy(buyStock, date, solutionAllInfo);

	int32_t todayStockNum = buyStock.size();

	const std::shared_ptr<StockMarket>& spMarket = avgFundHighScoreInfo->m_strategyInfo->m_spMarket;
	BigNumber chg = 0;
	solutionAllInfo->m_fund->stockChg(spMarket->stock(), spMarket->day(), chg);

	BigNumber score = 0;
	if (!m_spStrategy->sell(date, price, score, avgFundHighScoreInfo->m_strategyInfo))
	{
		if (todayStockNum > 0 && chg > "0.2")
		{
			++m_sellNum;
			if (m_sellNum > todayStockNum)
			{
				return false;
			}
			rate = 1;
			return true;
		}
		return false;
	}
	rate = score / "100.0";
	return true;
}

bool AvgFundHighScore::strategyBuy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionAllInfo>& solutionAllInfo)
{
	bool result = false;
	buyStock.clear();

	const std::shared_ptr<AvgFundHighScoreAllInfo>& avgFundHighScoreAllInfo = std::dynamic_pointer_cast<AvgFundHighScoreAllInfo>(solutionAllInfo);

	const std::vector<std::string>& filterStock = *avgFundHighScoreAllInfo->m_filterStock;
	int32_t index = -1;
	while (index++ != filterStock.size() - 1)
	{
		const std::string& stock = filterStock[index];
		BigNumber price;
		BigNumber score;
		const std::shared_ptr<StrategyInfo>& spStrategyInfo = avgFundHighScoreAllInfo->m_strategyAllInfo.find(stock)->second;
		if (m_spStrategy->buy(date,
			price,
			score,
			spStrategyInfo))
		{
			std::pair<std::string, std::pair<BigNumber, BigNumber>> choose;
			choose.first = stock;
			choose.second.first = price;
			choose.second.second = score;
			buyStock.push_back(choose);
			result = true;
		}
	}
	std::sort(buyStock.begin(), buyStock.end(), sortFun);
	return result;
}