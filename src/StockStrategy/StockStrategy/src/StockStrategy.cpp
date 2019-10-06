#include "StockStrategy.h"
#include "SarRiseBack.h"
#include "StockMysql/StockMysqlAPI.h"
#include "CStringManager/CStringManagerAPI.h"

StockStrategy::StockStrategy()
{

}

StockStrategy& StockStrategy::instance()
{
	static StockStrategy s_stockStrategy;
	return s_stockStrategy;
}

std::vector<std::string> StockStrategy::strategyAllStock()
{
	std::vector<std::string> result;
	//result.push_back("603988");
	//return result;
	std::vector<std::vector<std::string>> vecStocknameDb = StockMysql::instance().stockNameDb();
	int32_t lineIndex = -1;
	while (lineIndex++ != vecStocknameDb.size() - 1)
	{
		if (vecStocknameDb[lineIndex][2] == "0")
		{
			const std::string& stock = vecStocknameDb[lineIndex][0];
			if (CStringManager::Left(stock, 3) == "688")
			{
				continue;
			}
			result.push_back(vecStocknameDb[lineIndex][0]);
		}
	}
	return result;
}

std::shared_ptr<Strategy> StockStrategy::strategy(const std::string& stock,
	const std::shared_ptr<StockMarket>& stockMarket,
	const std::shared_ptr<StockWrIndicator>& stockWrIndicator,
	const std::shared_ptr<StockRsiIndicator>& stockRsiIndicator,
	const std::shared_ptr<StockSarIndicator>& stockSarIndicator,
	const std::shared_ptr<StockBollIndicator>& stockBollIndicator,
	StrategyEnum strategyEnum)
{
	std::shared_ptr<Strategy> strategy;
	switch (strategyEnum)
	{
	case SAR_RISE_BACK:
	{
		SarRiseBack* sarRiseBack = new SarRiseBack;
		sarRiseBack->init(stockMarket, stockSarIndicator, stockBollIndicator);
		strategy.reset(sarRiseBack);
	}
	break;
	default:
		break;
	}
	return strategy;
}