#include "AvgFundHighScore.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>
#include "AvgFundHighScoreAllInfo.h"
#include "StockFund/StockFundAPI.h"
#include "StockMarket/StockMarketAPI.h"

bool sortFun(const std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>& stock1,
	const std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>& stock2)
{
	if (stock1.second.second.first > stock2.second.second.first)
	{
		return true;
	}
	else if (stock1.second.second.first < stock2.second.second.first)
	{
		return false;
	}
	return stock1.second.second.second > stock2.second.second.second;
}

AvgFundHighScore::AvgFundHighScore():
m_stockNum(0),
m_minPollSize(0)
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
	buyStock.clear();
	std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>> strategyBuyStock;
	if (!strategyBuy(strategyBuyStock, date, solutionAllInfo))
	{
		return false;
	}

	int32_t index = -1;
	while (index++ != strategyBuyStock.size() - 1)
	{
		const std::string& stock = strategyBuyStock[index].first;
		RCSend("date = %s, stock = %s", date.dateToString().c_str(), stock.c_str());
	}

	if ((int32_t)strategyBuyStock.size() < m_minPollSize)
	{
		return false;
	}
	
	while (strategyBuyStock.size() > m_stockNum - solutionAllInfo->m_fund->allBuyInfo()->size())
	{
		strategyBuyStock.erase(--strategyBuyStock.end());
	}

	BigNumber allPercent = (int32_t)strategyBuyStock.size() * 100;
	index = -1;
	while (index++ != strategyBuyStock.size() - 1)
	{
		buyStock.push_back(std::pair<std::string, std::pair<BigNumber, BigNumber>>());
		std::pair<std::string, std::pair<BigNumber, BigNumber>>& backStock = buyStock.back();
		backStock.first = strategyBuyStock[index].first;
		backStock.second.first = strategyBuyStock[index].second.first;
		backStock.second.second = strategyBuyStock[index].second.second.first / allPercent.toPrec(2).zero();
		allPercent = allPercent - 100;
	}
	return true;
}

bool AvgFundHighScore::sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionAllInfo>& solutionAllInfo)
{
	sellStock.clear();
	std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>> strategySellStock;
	if (!strategySell(strategySellStock, date, solutionAllInfo))
	{
		return false;
	}

	std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>> strategyBuyStock;
	strategyBuy(strategyBuyStock, date, solutionAllInfo);

	int32_t index = -1;
	for (auto itSellStock = strategySellStock.begin(); itSellStock != strategySellStock.end(); ++itSellStock)
	{
		if ((++index < (int32_t)strategyBuyStock.size() && (int32_t)strategyBuyStock.size() >= m_minPollSize) ||
			itSellStock->second.second.second == 100)
		{
			sellStock.push_back(std::pair<std::string, std::pair<BigNumber, BigNumber>>());
			std::pair<std::string, std::pair<BigNumber, BigNumber>>& backStock = sellStock.back();
			backStock.first = itSellStock->first;
			backStock.second.first = itSellStock->second.first;
			backStock.second.second = itSellStock->second.second.first / "100.0";
			continue;
		}
	}
	return !sellStock.empty();
}

bool AvgFundHighScore::strategyBuy(std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>>& buyStock,
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
		BigNumber percent;
		BigNumber score;
		const std::shared_ptr<StrategyInfo>& spStrategyInfo = avgFundHighScoreAllInfo->m_strategyAllInfo.find(stock)->second;
		if (m_spStrategy->buy(date,
			price,
			percent,
			score,
			spStrategyInfo))
		{
			std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>> choose;
			choose.first = stock;
			choose.second.first = price;
			choose.second.second.first = percent;
			choose.second.second.second = score;
			buyStock.push_back(choose);
			result = true;
		}
	}
	std::sort(buyStock.begin(), buyStock.end(), sortFun);
	return result;
}

bool AvgFundHighScore::strategySell(std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>>& sellStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionAllInfo>& solutionAllInfo)
{
	bool result = false;
	sellStock.clear();
	std::vector<std::string> vecOwnedStock = solutionAllInfo->m_fund->ownedStock();

	int32_t index = -1;
	while (index++ != vecOwnedStock.size() - 1)
	{
		const std::string& stock = vecOwnedStock[index];
		auto& strategyInfo = solutionAllInfo->m_strategyAllInfo.find(stock)->second;
		auto& spMarket = strategyInfo->m_spMarket;
		BigNumber chg;
		solutionAllInfo->m_fund->stockChg(spMarket->stock(), spMarket->day(), chg);

		BigNumber price;
		BigNumber percent;
		BigNumber score;
		if (m_spStrategy->sell(date, price, percent, score, strategyInfo))
		{
			std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>> sellChoose;
			sellChoose.first = stock;
			sellChoose.second.first = price;
			sellChoose.second.second.first = percent;
			sellChoose.second.second.second = score;
			sellStock.push_back(sellChoose);
			result = true;
		}
	}
	std::sort(sellStock.begin(), sellStock.end(), sortFun);
	return result;
}