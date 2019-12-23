#include "CalcDateWrTask.h"
#include "StockIndicatorHelper.h"
#include "StockMarket/StockMarketAPI.h"

CalcDateWrTask::CalcDateWrTask():
m_indicatorData(nullptr)
{
	m_spMarket = nullptr;
}

void CalcDateWrTask::DoTask()
{
	m_spMarket->load();
	if (!m_spMarket->setDate(m_date))
	{
		return;
	}
	std::map<std::string, std::vector<std::vector<std::string>>>& indicatorData = *m_indicatorData;
	std::string stock = m_spMarket->stock();
	indicatorData[stock];
	auto& stockData = indicatorData.find(stock)->second;
	stockData.push_back(std::vector<std::string>());
	auto& stockDataBack = stockData.back();
	stockDataBack.push_back(m_date.dateToString());
	stockDataBack.push_back(StockIndicatorHelper::wr(10, m_date, *m_spMarket).toString());
	stockDataBack.push_back(StockIndicatorHelper::wr(20, m_date, *m_spMarket).toString());
}

void CalcDateWrTask::setParam(const IntDateTime& date,
	const std::shared_ptr<StockMarket>& spMarket,
	std::map<std::string, std::vector<std::vector<std::string>>>* indicatorData)
{
	m_date = date;
	m_spMarket = spMarket;
	m_indicatorData = indicatorData;
}