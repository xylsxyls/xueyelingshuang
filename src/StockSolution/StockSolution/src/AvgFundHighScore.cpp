#include "AvgFundHighScore.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>
#include "AvgFundHighScoreInfo.h"
#include "StockFund/StockFundAPI.h"
#include "StockMarket/StockMarketAPI.h"

static bool sortFun(const std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>& stock1,
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
m_stockNum(0)
{
	m_solutionType = AVG_FUND_HIGH_SCORE;
}

void AvgFundHighScore::init(int32_t stockNum)
{
	m_stockNum = stockNum;
}

bool AvgFundHighScore::buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	buyStock.clear();
	std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>> strategyBuyStock;
	if (!strategyBuy(strategyBuyStock, date, solutionInfo))
	{
		return false;
	}

	//RCSend("date = %s, stocksize = %d", date.dateToString().c_str(), strategyBuyStock.size());

	if (strategyBuyCount(date, solutionInfo) < minPollSize(solutionInfo))
	{
		return false;
	}

	int32_t firstSize = strategyBuyStock.size();
	int32_t popBuySize = popSize(firstSize, solutionInfo);
	//int32_t popSize = firstSize <= 11 ? (firstSize - firstSize / 2) : (firstSize - 5);
	//int32_t popSize = firstSize <= 5 ? 0 : (firstSize - 5);
	while (popBuySize-- != 0)
	{
		strategyBuyStock.pop_back();
	}
	if (strategyBuyStock.size() == 0)
	{
		return false;
	}

	auto strategyBuyStockTemp = strategyBuyStock;
	
	while (strategyBuyStockTemp.size() > m_stockNum - solutionInfo->m_fund->allBuyInfo()->size())
	{
		strategyBuyStockTemp.erase(--strategyBuyStockTemp.end());
	}
	//(m_stockNum - solutionInfo->m_fund->allBuyInfo()->size())
	//strategyBuyStock.size()
	BigNumber allPercent = (int32_t)strategyBuyStockTemp.size() * 100;
	int32_t index = -1;
	while (index++ != strategyBuyStock.size() - 1)
	{
		buyStock.push_back(std::pair<std::string, std::pair<BigNumber, BigNumber>>());
		std::pair<std::string, std::pair<BigNumber, BigNumber>>& backStock = buyStock.back();
		backStock.first = strategyBuyStock[index].first;
		backStock.second.first = strategyBuyStock[index].second.first;
		backStock.second.second = strategyBuyStock[index].second.second.first / allPercent.toPrec(2).zero();
		if (allPercent == 0)
		{
			continue;
		}
		allPercent = allPercent - 100;
	}
	return true;
}

bool AvgFundHighScore::sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	sellStock.clear();
	std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>> strategySellStock;
	if (!strategySell(strategySellStock, date, solutionInfo))
	{
		return false;
	}

	std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>> strategyBuyStock;
	strategyBuy(strategyBuyStock, date, solutionInfo);

	int32_t strategyCount = strategyBuyCount(date, solutionInfo);

	int32_t index = -1;
	for (auto itSellStock = strategySellStock.begin(); itSellStock != strategySellStock.end(); ++itSellStock)
	{
		if (itSellStock->second.second.second == 100)
		{
			sellStock.push_back(std::pair<std::string, std::pair<BigNumber, BigNumber>>());
			std::pair<std::string, std::pair<BigNumber, BigNumber>>& backStock = sellStock.back();
			backStock.first = itSellStock->first;
			backStock.second.first = itSellStock->second.first;
			backStock.second.second = itSellStock->second.second.first / "100.0";
			continue;
		}
		else if (itSellStock->second.second.second == 90 &&
			strategyCount >= minPollSize(solutionInfo) &&
			++index < (int32_t)strategyBuyStock.size())
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
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	bool result = false;
	buyStock.clear();

	const std::shared_ptr<AvgFundHighScoreInfo>& avgFundHighScoreInfo = std::dynamic_pointer_cast<AvgFundHighScoreInfo>(solutionInfo);

	const std::vector<std::string>& filterStock = *avgFundHighScoreInfo->m_filterStock;
	int32_t index = -1;
	while (index++ != filterStock.size() - 1)
	{
		const std::string& stock = filterStock[index];
		BigNumber price;
		BigNumber percent;
		BigNumber score;
		const std::shared_ptr<StrategyInfo>& spStrategyInfo = avgFundHighScoreInfo->m_strategyAllInfo.find(stock)->second.begin()->second.first;
		if (m_vecStrategy[0]->buy(date,
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

int32_t AvgFundHighScore::strategyBuyCount(const IntDateTime& date, const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	const std::shared_ptr<AvgFundHighScoreInfo>& avgFundHighScoreInfo = std::dynamic_pointer_cast<AvgFundHighScoreInfo>(solutionInfo);

	int32_t count = 0;
	const std::vector<std::string>& filterStock = *avgFundHighScoreInfo->m_filterStock;
	int32_t index = -1;
	while (index++ != filterStock.size() - 1)
	{
		const std::string& stock = filterStock[index];
		BigNumber price;
		BigNumber percent;
		BigNumber score;
		const std::shared_ptr<StrategyInfo>& spStrategyInfo = avgFundHighScoreInfo->m_strategyAllInfo.find(stock)->second.begin()->second.second;
		count += (int32_t)m_vecStrategy[1]->buy(date, price, percent, score, spStrategyInfo);
	}
	return count;
}

bool AvgFundHighScore::strategySell(std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>>& sellStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	bool result = false;
	sellStock.clear();
	std::vector<std::string> vecOwnedStock = solutionInfo->m_fund->ownedStock();

	int32_t index = -1;
	while (index++ != vecOwnedStock.size() - 1)
	{
		const std::string& stock = vecOwnedStock[index];
		auto& strategyInfo = solutionInfo->m_strategyAllInfo.find(stock)->second.begin()->second.first;
		auto& spMarket = strategyInfo->m_spMarket;

		BigNumber price;
		BigNumber percent;
		BigNumber score;
		if (m_vecStrategy[0]->sell(date, price, percent, score, strategyInfo))
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

int32_t AvgFundHighScore::minPollSize(const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	if (solutionInfo->m_strategyAllInfo.empty())
	{
		return 0;
	}
	if (solutionInfo->m_strategyAllInfo.begin()->second.empty())
	{
		return 0;
	}
	return solutionInfo->m_strategyAllInfo.begin()->second.begin()->second.first->minPollSize();
}

int32_t AvgFundHighScore::popSize(int32_t buySize, const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	if (solutionInfo->m_strategyAllInfo.empty())
	{
		return 0;
	}
	if (solutionInfo->m_strategyAllInfo.begin()->second.empty())
	{
		return 0;
	}
	return solutionInfo->m_strategyAllInfo.begin()->second.begin()->second.first->popSize(buySize);
}