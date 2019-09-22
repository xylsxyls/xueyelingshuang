#include "StockIndicator.h"
#include "StockWrIndicator.h"
#include "StockRsiIndicator.h"
#include "StockAvgIndicator.h"
#include "StockMysql/StockMysqlAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicatorHelper.h"
#include "StockAvg.h"

StockIndicator::StockIndicator()
{

}

StockIndicator& StockIndicator::instance()
{
	static StockIndicator s_stockIndicator;
	return s_stockIndicator;
}

void StockIndicator::loadIndicatorFromRedis(const std::string& stock, const IntDateTime& beginTime, const IntDateTime& endTime)
{
	m_loadStock = stock;
	m_indicatorIndex = StockMysql::instance().getIndicatorDataIndex();
	m_redisIndicatorData = StockMysql::instance().readIndicator(stock, beginTime, endTime);
}

void StockIndicator::loadCalcFromRedis(const std::string& stock, const IntDateTime& beginTime, const IntDateTime& endTime)
{
	m_loadStock = stock;
	m_calcIndex = StockMysql::instance().getCalcDataIndex();
	m_redisCalcData = StockMysql::instance().readCalc(stock, beginTime, endTime);
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
		auto& stockData = indicatorData.find(stock)->second;
		do 
		{
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
		indicatorData[stock];
		StockMarket market;
		market.loadFromMysql(stock);
		market.load();
		std::map<IntDateTime, BigNumber> rsi6Indicator = StockIndicatorHelper::rsiTongHuaShun(6, market);
		std::map<IntDateTime, BigNumber> rsi12Indicator = StockIndicatorHelper::rsiTongHuaShun(12, market);
		std::map<IntDateTime, BigNumber> rsi24Indicator = StockIndicatorHelper::rsiTongHuaShun(24, market);

		auto& stockData = indicatorData.find(stock)->second;
		auto itRsi6 = rsi6Indicator.begin();
		auto itRsi12 = rsi12Indicator.begin();
		auto itRsi24 = rsi24Indicator.begin();
		for (; itRsi6 != rsi6Indicator.end();)
		{
			indicatorData[stock].push_back(std::vector<std::string>());
			std::vector<std::string>& vecLine = stockData.back();
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

void StockIndicator::saveDateWr(const IntDateTime& date)
{
	std::map<std::string, std::vector<std::vector<std::string>>> indicatorData;
	std::vector<std::string> allStock = StockMysql::instance().allStockFromMysql();
	int32_t index = -1;
	while (index++ != allStock.size() - 1)
	{
		RCSend("calcWr = %d", index + 1);
		const std::string& stock = allStock[index];
		StockMarket market;
		market.loadFromMysql(stock);
		market.load();
		if (!market.setDate(date))
		{
			continue;
		}
		indicatorData[stock];
		auto& stockData = indicatorData.find(stock)->second;
		stockData.push_back(std::vector<std::string>());
		auto& stockDataBack = stockData.back();
		stockDataBack.push_back(date.dateToString());
		stockDataBack.push_back(StockIndicatorHelper::wr(10, date, market).toString());
		stockDataBack.push_back(StockIndicatorHelper::wr(20, date, market).toString());
	}
	StockMysql::instance().saveIndicator("wr", "date,wr10,wr20", indicatorData, true);
}

void StockIndicator::saveDateRsi(const IntDateTime& date)
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
		if (!market.setDate(date))
		{
			continue;
		}
		indicatorData[stock];
		
		std::map<IntDateTime, BigNumber> rsi6Indicator = StockIndicatorHelper::rsiTongHuaShun(6, market);
		std::map<IntDateTime, BigNumber> rsi12Indicator = StockIndicatorHelper::rsiTongHuaShun(12, market);
		std::map<IntDateTime, BigNumber> rsi24Indicator = StockIndicatorHelper::rsiTongHuaShun(24, market);

		auto& stockData = indicatorData.find(stock)->second;
		stockData.push_back(std::vector<std::string>());
		std::vector<std::string>& vecLine = stockData.back();
		vecLine.push_back(date.dateToString());
		vecLine.push_back(rsi6Indicator.rbegin()->second.toString());
		vecLine.push_back(rsi12Indicator.rbegin()->second.toString());
		vecLine.push_back(rsi24Indicator.rbegin()->second.toString());
	}
	StockMysql::instance().saveIndicator("rsi", "date,rsi6,rsi12,rsi24", indicatorData, true);
}

void StockIndicator::saveAvg(const std::string& stock, const std::map<IntDateTime, std::shared_ptr<StockAvg>>& avgData)
{
	std::map<std::string, std::vector<int32_t>> calcDataIndex = StockMysql::instance().getCalcDataIndex();
	if (calcDataIndex.empty())
	{
		StockMysql::instance().saveCalcDataIndex();
		calcDataIndex = StockMysql::instance().getCalcDataIndex();
	}
	int32_t dateIndex = calcDataIndex.find("date")->second[0];
	const std::vector<int32_t>& calcIndex = calcDataIndex.find("avg")->second;

	std::map<IntDateTime, std::vector<std::string>> saveCalcData;
	for (auto itDate = avgData.begin(); itDate != avgData.end(); ++itDate)
	{
		const IntDateTime& date = itDate->first;
		const std::shared_ptr<StockAvg>& stockAvg = itDate->second;
		saveCalcData[date];
		std::shared_ptr<std::vector<std::vector<std::string>>> dateData = StockMysql::instance().readCalc(stock, date, date);
		std::vector<std::string>& saveCalcDateData = saveCalcData.find(date)->second;
		if (dateData == nullptr)
		{
			saveCalcDateData.push_back(stockAvg->m_avg09_30.toString());
			saveCalcDateData.push_back(stockAvg->m_avg10_00.toString());
			saveCalcDateData.push_back(stockAvg->m_avg10_30.toString());
			saveCalcDateData.push_back(stockAvg->m_avg11_00.toString());
			saveCalcDateData.push_back(stockAvg->m_avg11_30.toString());
			saveCalcDateData.push_back(stockAvg->m_avg13_30.toString());
			saveCalcDateData.push_back(stockAvg->m_avg14_00.toString());
			saveCalcDateData.push_back(stockAvg->m_avg14_30.toString());
			saveCalcDateData.push_back(stockAvg->m_avg15_00.toString());
			saveCalcDateData.push_back(stockAvg->m_avgHigh.toString());
			saveCalcDateData.push_back(stockAvg->m_avgLow.toString());
		}
		else
		{
			saveCalcDateData = (*dateData)[0];
			saveCalcDateData.erase(saveCalcDateData.begin());
			saveCalcDateData[calcIndex[0] - 1] = stockAvg->m_avg09_30.toString();
			saveCalcDateData[calcIndex[1] - 1] = stockAvg->m_avg10_00.toString();
			saveCalcDateData[calcIndex[2] - 1] = stockAvg->m_avg10_30.toString();
			saveCalcDateData[calcIndex[3] - 1] = stockAvg->m_avg11_00.toString();
			saveCalcDateData[calcIndex[4] - 1] = stockAvg->m_avg11_30.toString();
			saveCalcDateData[calcIndex[5] - 1] = stockAvg->m_avg13_30.toString();
			saveCalcDateData[calcIndex[6] - 1] = stockAvg->m_avg14_00.toString();
			saveCalcDateData[calcIndex[7] - 1] = stockAvg->m_avg14_30.toString();
			saveCalcDateData[calcIndex[8] - 1] = stockAvg->m_avg15_00.toString();
			saveCalcDateData[calcIndex[9] - 1] = stockAvg->m_avgHigh.toString();
			saveCalcDateData[calcIndex[10] - 1] = stockAvg->m_avgLow.toString();
		}
	}
	StockMysql::instance().saveCalc(stock, saveCalcData);
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

std::shared_ptr<StockAvgIndicator> StockIndicator::avg()
{
	const std::vector<int32_t>& calcIndex = m_calcIndex.find("avg")->second;
	int32_t dateIndex = m_calcIndex.find("date")->second[0];
	std::shared_ptr<StockAvgIndicator> spStockAvgIndicator(new StockAvgIndicator);
	spStockAvgIndicator->setRedisData(m_loadStock, m_redisCalcData, dateIndex, calcIndex);
	return spStockAvgIndicator;
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