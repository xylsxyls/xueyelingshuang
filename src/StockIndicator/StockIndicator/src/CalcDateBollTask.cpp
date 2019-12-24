#include "CalcDateBollTask.h"
#include "StockIndicatorHelper.h"
#include "StockMarket/StockMarketAPI.h"

CalcDateBollTask::CalcDateBollTask() :
m_indicatorData(nullptr)
{

}

void CalcDateBollTask::DoTask()
{
	if (!m_market.setDate(m_date))
	{
		return;
	}
	std::map<std::string, std::vector<std::vector<std::string>>>& indicatorData = *m_indicatorData;
	std::string stock = m_market.stock();
	indicatorData[stock];
	auto& stockData = indicatorData.find(stock)->second;
	stockData.push_back(std::vector<std::string>());
	auto& stockDataBack = stockData.back();
	stockDataBack.push_back(m_date.dateToString());
	std::pair<BigNumber, std::pair<BigNumber, BigNumber>> dayBoll = StockIndicatorHelper::boll(m_date, m_market);
	stockDataBack.push_back(dayBoll.first.toString());
	stockDataBack.push_back(dayBoll.second.first.toString());
	stockDataBack.push_back(dayBoll.second.second.toString());
}

void CalcDateBollTask::setParam(const IntDateTime& date,
	const StockMarket& market,
	std::map<std::string, std::vector<std::vector<std::string>>>* indicatorData)
{
	m_date = date;
	m_market = market;
	m_indicatorData = indicatorData;
}