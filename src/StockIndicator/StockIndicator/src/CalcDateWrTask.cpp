#include "CalcDateWrTask.h"
#include "StockIndicatorHelper.h"
#include "StockMarket/StockMarketAPI.h"

CalcDateWrTask::CalcDateWrTask():
m_indicatorData(nullptr)
{

}

void CalcDateWrTask::DoTask()
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
	stockDataBack.push_back(StockIndicatorHelper::wr(10, m_date, m_market).toString());
	stockDataBack.push_back(StockIndicatorHelper::wr(20, m_date, m_market).toString());
}

void CalcDateWrTask::setParam(const IntDateTime& date,
	const StockMarket& market,
	std::map<std::string, std::vector<std::vector<std::string>>>* indicatorData)
{
	m_date = date;
	m_market = market;
	m_indicatorData = indicatorData;
}