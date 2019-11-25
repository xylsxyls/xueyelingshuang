#include "AvgFundHighScore.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>
#include "AvgFundHighScoreAllInfo.h"
#include "AvgFundHighScoreInfo.h"

bool sortFun(const std::pair<std::string, std::pair<BigNumber, BigNumber>>& stock1,
	const std::pair<std::string, std::pair<BigNumber, BigNumber>>& stock2)
{
	return stock1.second.second > stock2.second.second;
}

bool AvgFundHighScore::buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
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
		if (m_spStrategy->buy(date,
			price,
			score,
			avgFundHighScoreAllInfo->m_strategyAllInfo.find(stock)->second))
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

	if (buyStock.size() < 4)
	{
		buyStock.clear();
		return false;
	}

	BigNumber allScore = 0;
	index = -1;
	while (index++ != (std::min)((int32_t)buyStock.size(), 5) - 1)
	{
		allScore = allScore + buyStock[index].second.second;
	}
	index = -1;
	while (index++ != buyStock.size() - 1)
	{
		BigNumber score = buyStock[index].second.second;
		buyStock[index].second.second = score / BigNumber((std::max)(atoi(allScore.toString().c_str()), 100)).toPrec(2).zero();
		allScore = allScore - score;
	}
	return result;
}

bool AvgFundHighScore::sell(const IntDateTime& date,
	BigNumber& price,
	BigNumber& rate,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	const std::shared_ptr<AvgFundHighScoreInfo>& avgFundHighScoreInfo = std::dynamic_pointer_cast<AvgFundHighScoreInfo>(solutionInfo);
	BigNumber score = 0;
	if (!m_spStrategy->sell(date, price, score, avgFundHighScoreInfo->m_strategyInfo))
	{
		return false;
	}
	rate = score / "100.0";
	return true;
}