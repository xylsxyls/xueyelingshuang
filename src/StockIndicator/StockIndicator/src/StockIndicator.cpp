#include "StockIndicator.h"
#include "StockWrIndicator.h"
#include "StockRsiIndicator.h"
#include "StockMysql/StockMysqlAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicatorHelper.h"

StockIndicator::StockIndicator()
{

}

StockIndicator& StockIndicator::instance()
{
	static StockIndicator s_stockIndicator;
	return s_stockIndicator;
}

void StockIndicator::loadFromRedis(const std::string& stock, const IntDateTime& beginTime, const IntDateTime& endTime)
{
	m_indicatorIndex = StockMysql::instance().getIndicatorDataIndex();
	m_loadStock = stock;
	m_redisIndicatorData = StockMysql::instance().readIndicator(stock, beginTime, endTime);
}

void StockIndicator::clear()
{
	m_indicatorIndex.clear();
	m_loadStock.clear();
	m_redisIndicatorData = nullptr;
}

void StockIndicator::saveWr()
{
	std::map<std::string, std::vector<std::vector<std::string>>> indicatorData;
	std::vector<std::string> allStock = StockMysql::instance().allStockFromMysql();
	int32_t index = -1;
	while (index++ != allStock.size() - 1)
	{
		RCSend("calcWr = %d", index + 1);
		const std::string& stock = allStock[index];
		indicatorData[stock];
		StockMarket market;
		market.loadFromMysql(stock);
		market.load();
		if (market.empty())
		{
			continue;
		}
		StockMarket marketCalc = market;
		do 
		{
			auto& stockData = indicatorData.find(stock)->second;
			stockData.push_back(std::vector<std::string>());
			auto& stockDataBack = stockData.back();
			stockDataBack.push_back(market.date().dateToString());
			stockDataBack.push_back(StockIndicatorHelper::wr(10, market.date(), marketCalc).toString());
			stockDataBack.push_back(StockIndicatorHelper::wr(20, market.date(), marketCalc).toString());
		} while (market.next());
	}
	StockMysql::instance().saveIndicator("wr", "date,wr10,wr20", indicatorData);
}

void StockIndicator::saveRsi()
{
	std::map<std::string, std::vector<std::vector<std::string>>> indicatorData;
	std::vector<std::string> allStock = StockMysql::instance().allStockFromMysql();
	int32_t index = -1;
	while (index++ != allStock.size() - 1)
	{
		RCSend("calcRsi = %d", index + 1);
		const std::string& stock = allStock[index];
		StockMarket market;
		market.loadFromMysql(stock);
		market.load();
		std::map<IntDateTime, BigNumber> rsi6Indicator = StockIndicatorHelper::rsiTongHuaShun(6, market);
		std::map<IntDateTime, BigNumber> rsi12Indicator = StockIndicatorHelper::rsiTongHuaShun(12, market);
		std::map<IntDateTime, BigNumber> rsi24Indicator = StockIndicatorHelper::rsiTongHuaShun(24, market);

		auto itRsi6 = rsi6Indicator.begin();
		auto itRsi12 = rsi12Indicator.begin();
		auto itRsi24 = rsi24Indicator.begin();
		for (; itRsi6 != rsi6Indicator.end();)
		{
			indicatorData[stock].push_back(std::vector<std::string>());
			std::vector<std::string>& vecLine = indicatorData.find(stock)->second.back();
			vecLine.push_back(itRsi6->first.dateToString());
			vecLine.push_back(itRsi6->second.toString());
			vecLine.push_back(itRsi12->second.toString());
			vecLine.push_back(itRsi24->second.toString());
			++itRsi6;
			++itRsi12;
			++itRsi24;
		}
	}
	StockMysql::instance().saveIndicator("rsi", "date,rsi6,rsi12,rsi24", indicatorData);
}

std::shared_ptr<StockWrIndicator> StockIndicator::wr()
{
	const std::vector<int32_t>& indicatorIndex = m_indicatorIndex.find("wr")->second;
	int32_t dateIndex = m_indicatorIndex.find("date")->second[0];
	std::shared_ptr<StockWrIndicator> spStockWrIndicator(new StockWrIndicator);
	spStockWrIndicator->setRedisData(m_loadStock, m_redisIndicatorData, dateIndex, indicatorIndex);
	return spStockWrIndicator;
}

std::shared_ptr<StockRsiIndicator> StockIndicator::rsi()
{
	const std::vector<int32_t>& indicatorIndex = m_indicatorIndex.find("rsi")->second;
	int32_t dateIndex = m_indicatorIndex.find("date")->second[0];
	std::shared_ptr<StockRsiIndicator> spStockRsiIndicator(new StockRsiIndicator);
	spStockRsiIndicator->setRedisData(m_loadStock, m_redisIndicatorData, dateIndex, indicatorIndex);
	return spStockRsiIndicator;
}

//int main()
//{
//	StockWrIndicator in;
//	in.load("603826", "2019-01-01", "2019-04-30");
//
//	StockRsiIndicator in2;
//	in2.load("603826", "2019-01-01", "2019-04-30");
//	return 0;
//}