#include "StockIndicator.h"
#include "StockWrIndicator.h"
#include "StockRsiIndicator.h"
#include "StockSarIndicator.h"
#include "StockBollIndicator.h"
#include "StockAvgIndicator.h"
#include "StockMysql/StockMysqlAPI.h"
#include "StockIndicatorHelper.h"
#include "StockAvg.h"
#include "StockSar.h"
#include "StockBoll.h"
#include "CalcDateWrTask.h"
#include "CalcDateRsiTask.h"
#include "CalcDateSarTask.h"
#include "CalcDateBollTask.h"
#include "CSystem/CSystemAPI.h"

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

void StockIndicator::saveSar()
{
	std::vector<std::string> allStock = StockMysql::instance().allStockFromMysql();
	std::vector<std::vector<std::string>> vecAllStock = split(allStock);
	int32_t num = 0;
	int32_t groupIndex = -1;
	while (groupIndex++ != vecAllStock.size() - 1)
	{
		std::map<std::string, std::vector<std::vector<std::string>>> indicatorData;
		int32_t index = -1;
		while (index++ != vecAllStock[groupIndex].size() - 1)
		{
			RCSend("calcSar = %d", ++num);
			const std::string& stock = vecAllStock[groupIndex][index];
			indicatorData[stock];
			StockMarket market;
			market.loadFromMysql(stock);
			market.load();
			std::map<IntDateTime, std::pair<BigNumber, int32_t>> sar5Indicator = StockIndicatorHelper::sar(market, 5);
			std::map<IntDateTime, std::pair<BigNumber, int32_t>> sar10Indicator = StockIndicatorHelper::sar(market, 10);
			std::map<IntDateTime, std::pair<BigNumber, int32_t>> sar20Indicator = StockIndicatorHelper::sar(market, 20);

			auto& stockData = indicatorData.find(stock)->second;
			auto itSar5 = sar5Indicator.begin();
			auto itSar10 = sar10Indicator.begin();
			auto itSar20 = sar20Indicator.begin();
			for (; itSar5 != sar5Indicator.end();)
			{
				indicatorData[stock].push_back(std::vector<std::string>());
				std::vector<std::string>& vecLine = stockData.back();
				vecLine.push_back(itSar5->first.dateToString());
				vecLine.push_back(itSar5->second.first.toString());
				vecLine.push_back(std::to_string(itSar5->second.second));
				vecLine.push_back(itSar10->second.first.toString());
				vecLine.push_back(std::to_string(itSar10->second.second));
				vecLine.push_back(itSar20->second.first.toString());
				vecLine.push_back(std::to_string(itSar20->second.second));
				++itSar5;
				++itSar10;
				++itSar20;
			}
		}
		StockMysql::instance().saveIndicator("sar", "date,sar5,state5,sar10,state10,sar20,state20", indicatorData, groupIndex != 0);
	}
}

void StockIndicator::saveBoll()
{
	std::map<std::string, std::vector<std::vector<std::string>>> indicatorData;
	std::vector<std::string> allStock = StockMysql::instance().allStockFromMysql();
	int32_t index = -1;
	while (index++ != allStock.size() - 1)
	{
		RCSend("calcBoll = %d", index + 1);
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
			IntDateTime date = market.date();
			stockDataBack.push_back(date.dateToString());
			std::pair<BigNumber, std::pair<BigNumber, BigNumber>> dayBoll = StockIndicatorHelper::boll(date, marketCalc);
			stockDataBack.push_back(dayBoll.first.toString());
			stockDataBack.push_back(dayBoll.second.first.toString());
			stockDataBack.push_back(dayBoll.second.second.toString());
		} while (market.next());
	}
	StockMysql::instance().saveIndicator("boll", "date,bollmid,bollup,bolldown", indicatorData);
}

void StockIndicator::dateIndicator(const IntDateTime& date,
	std::map<std::string, std::vector<std::vector<std::string>>>& wrIndicatorData,
	std::map<std::string, std::vector<std::vector<std::string>>>& rsiIndicatorData,
	std::map<std::string, std::vector<std::vector<std::string>>>& sarIndicatorData,
	std::map<std::string, std::vector<std::vector<std::string>>>& bollIndicatorData,
	const std::vector<std::string>& allStock,
	bool loadFromMysql,
	const std::vector<uint32_t>& vecThreadId)
{
	wrIndicatorData.clear();
	rsiIndicatorData.clear();
	sarIndicatorData.clear();
	bollIndicatorData.clear();
	int32_t threadIdCount = vecThreadId.size();
	int32_t index = -1;
	while (index++ != allStock.size() - 1)
	{
		RCSend("calcIndicator = %d", index + 1);

		StockMarket market;
		if (loadFromMysql)
		{
			market.loadFromMysql(allStock[index]);
		}
		else
		{
			market.loadFromRedis(allStock[index]);
		}
		market.load();

		std::shared_ptr<CalcDateWrTask> spCalcDateWrTask(new CalcDateWrTask);
		spCalcDateWrTask->setParam(date, market, &wrIndicatorData);
		CTaskThreadManager::Instance().GetThreadInterface(vecThreadId[index % threadIdCount])->PostTask(spCalcDateWrTask);

		std::shared_ptr<CalcDateRsiTask> spCalcDateRsiTask(new CalcDateRsiTask);
		spCalcDateRsiTask->setParam(date, market, &rsiIndicatorData);
		CTaskThreadManager::Instance().GetThreadInterface(vecThreadId[index % threadIdCount])->PostTask(spCalcDateRsiTask);

		std::shared_ptr<CalcDateSarTask> spCalcDateSarTask(new CalcDateSarTask);
		spCalcDateSarTask->setParam(date, market, &sarIndicatorData);
		CTaskThreadManager::Instance().GetThreadInterface(vecThreadId[index % threadIdCount])->PostTask(spCalcDateSarTask);

		std::shared_ptr<CalcDateBollTask> spCalcDateBollTask(new CalcDateBollTask);
		spCalcDateBollTask->setParam(date, market, &bollIndicatorData);
		CTaskThreadManager::Instance().GetThreadInterface(vecThreadId[index % threadIdCount])->PostTask(spCalcDateBollTask);
	}
}

void StockIndicator::saveDateIndicator(const IntDateTime& date)
{
	std::vector<std::string> allStock = StockMysql::instance().allStockFromMysql();

	std::vector<uint32_t> vecThreadId;
	int32_t coreCount = CSystem::GetCPUCoreCount();
	int32_t index = -1;
	while (index++ != coreCount - 1)
	{
		vecThreadId.push_back(CTaskThreadManager::Instance().Init());
	}

	dateIndicator(date, m_wrIndicatorData, m_rsiIndicatorData, m_sarIndicatorData, m_bollIndicatorData, allStock, true, vecThreadId);

	index = -1;
	while (index++ != coreCount - 1)
	{
		CTaskThreadManager::Instance().WaitForEnd(vecThreadId[index]);
	}

	StockMysql::instance().saveIndicator("wr", "date,wr10,wr20", m_wrIndicatorData, true);
	StockMysql::instance().saveIndicator("rsi", "date,rsi6,rsi12,rsi24", m_rsiIndicatorData, true);
	StockMysql::instance().saveIndicator("sar", "date,sar5,state5,sar10,state10,sar20,state20", m_sarIndicatorData, true);
	StockMysql::instance().saveIndicator("boll", "date,bollmid,bollup,bolldown", m_bollIndicatorData, true);
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

void StockIndicator::updateDateIndicatorToRedis(const IntDateTime& date, bool useLast)
{
	std::vector<std::string> allStock = StockMysql::instance().allStock();

	std::vector<uint32_t> vecThreadId;
	int32_t coreCount = CSystem::GetCPUCoreCount();
	int32_t index = -1;
	while (index++ != coreCount - 1)
	{
		vecThreadId.push_back(CTaskThreadManager::Instance().Init());
	}

	if (!useLast || m_wrIndicatorData.empty())
	{
		dateIndicator(date, m_wrIndicatorData, m_rsiIndicatorData, m_sarIndicatorData, m_bollIndicatorData, allStock, false, vecThreadId);
	}

	index = -1;
	while (index++ != coreCount - 1)
	{
		CTaskThreadManager::Instance().WaitForEnd(vecThreadId[index]);
	}

	std::map<std::string, std::map<std::string, std::vector<std::string>>> allIndicatorData;

	auto itWrIndicator = m_wrIndicatorData.begin();
	auto itRsiIndicator = m_rsiIndicatorData.begin();
	auto itSarIndicator = m_sarIndicatorData.begin();
	auto itBollIndicator = m_bollIndicatorData.begin();
	for (; itWrIndicator != m_wrIndicatorData.end();)
	{
		itWrIndicator->second[0].erase(itWrIndicator->second[0].begin());
		itRsiIndicator->second[0].erase(itRsiIndicator->second[0].begin());
		itSarIndicator->second[0].erase(itSarIndicator->second[0].begin());
		itBollIndicator->second[0].erase(itBollIndicator->second[0].begin());

		allIndicatorData[itWrIndicator->first]["wr"] = itWrIndicator->second[0];
		allIndicatorData[itRsiIndicator->first]["rsi"] = itRsiIndicator->second[0];
		allIndicatorData[itSarIndicator->first]["sar"] = itSarIndicator->second[0];
		allIndicatorData[itBollIndicator->first]["boll"] = itBollIndicator->second[0];

		++itWrIndicator;
		++itRsiIndicator;
		++itSarIndicator;
		++itBollIndicator;
	}
	
	StockMysql::instance().updateDateIndicatorToRedis(date, allIndicatorData);
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

std::shared_ptr<StockSarIndicator> StockIndicator::sar()
{
	const std::vector<int32_t>& indicatorIndex = m_indicatorIndex.find("sar")->second;
	int32_t dateIndex = m_indicatorIndex.find("date")->second[0];
	std::shared_ptr<StockSarIndicator> spStockSarIndicator(new StockSarIndicator);
	spStockSarIndicator->setRedisData(m_loadStock, m_redisIndicatorData, dateIndex, indicatorIndex);
	return spStockSarIndicator;
}

std::shared_ptr<StockBollIndicator> StockIndicator::boll()
{
	const std::vector<int32_t>& indicatorIndex = m_indicatorIndex.find("boll")->second;
	int32_t dateIndex = m_indicatorIndex.find("date")->second[0];
	std::shared_ptr<StockBollIndicator> spStockBollIndicator(new StockBollIndicator);
	spStockBollIndicator->setRedisData(m_loadStock, m_redisIndicatorData, dateIndex, indicatorIndex);
	return spStockBollIndicator;
}

std::shared_ptr<StockAvgIndicator> StockIndicator::avg()
{
	const std::vector<int32_t>& calcIndex = m_calcIndex.find("avg")->second;
	int32_t dateIndex = m_calcIndex.find("date")->second[0];
	std::shared_ptr<StockAvgIndicator> spStockAvgIndicator(new StockAvgIndicator);
	spStockAvgIndicator->setRedisData(m_loadStock, m_redisCalcData, dateIndex, calcIndex);
	return spStockAvgIndicator;
}

std::vector<std::vector<std::string>> StockIndicator::split(const std::vector<std::string>& allStock, int32_t num)
{
	std::vector<std::vector<std::string>> result;
	int32_t index = -1;
	while (true)
	{
		int32_t count = num;
		result.push_back(std::vector<std::string>());
		while (count-- != 0)
		{
			result.back().push_back(allStock[++index]);
			if (index == allStock.size() - 1)
			{
				return result;
			}
		}
	}
	return result;
}

//int main()
//{
//	StockMarket stockMarket;
//	stockMarket.loadFromMysql("603826");
//	stockMarket.load();
//	StockIndicator::instance().loadIndicatorFromRedis("603826");
//	std::shared_ptr<StockSarIndicator> spSar = StockIndicator::instance().sar();
//	spSar->load();
//	printf("%s\n", spSar->day("2019-09-30")->m_sar5.toString().c_str());
//	printf("%s\n", spSar->day("2019-09-30")->m_sar10.toString().c_str());
//	printf("%s\n", spSar->day("2019-09-30")->m_sar20.toString().c_str());
//	printf("%d\n", spSar->day("2019-09-30")->m_sarState5);
//	printf("%d\n", spSar->day("2019-09-30")->m_sarState10);
//	printf("%d\n", spSar->day("2019-09-30")->m_sarState20);
//	std::shared_ptr<StockBollIndicator> spBoll = StockIndicator::instance().boll();
//	spBoll->load();
//	printf("%s\n", spBoll->day("2019-09-30")->m_mid.toString().c_str());
//	printf("%s\n", spBoll->day("2019-09-30")->m_up.toString().c_str());
//	printf("%s\n", spBoll->day("2019-09-30")->m_down.toString().c_str());
//	auto boll = StockIndicatorHelper::wr(20,"2019-09-30", stockMarket);
//	printf("%s", boll.toString().c_str());
//	getchar();
//	return 0;
//}