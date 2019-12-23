#include "CalcDateRsiTask.h"
#include "StockIndicatorHelper.h"
#include "StockMarket/StockMarketAPI.h"

CalcDateRsiTask::CalcDateRsiTask() :
m_indicatorData(nullptr)
{
	m_spMarket = nullptr;
}

void CalcDateRsiTask::DoTask()
{
	m_spMarket->load();
	if (!m_spMarket->setDate(m_date))
	{
		return;
	}
	std::map<std::string, std::vector<std::vector<std::string>>>& indicatorData = *m_indicatorData;
	std::string stock = m_spMarket->stock();
	indicatorData[stock];

	std::map<IntDateTime, BigNumber> rsi6Indicator = StockIndicatorHelper::rsiTongHuaShun(6, *m_spMarket);
	std::map<IntDateTime, BigNumber> rsi12Indicator = StockIndicatorHelper::rsiTongHuaShun(12, *m_spMarket);
	std::map<IntDateTime, BigNumber> rsi24Indicator = StockIndicatorHelper::rsiTongHuaShun(24, *m_spMarket);

	auto& stockData = indicatorData.find(stock)->second;
	stockData.push_back(std::vector<std::string>());
	std::vector<std::string>& vecLine = stockData.back();
	vecLine.push_back(m_date.dateToString());
	vecLine.push_back(rsi6Indicator.find(m_date)->second.toString());
	vecLine.push_back(rsi12Indicator.find(m_date)->second.toString());
	vecLine.push_back(rsi24Indicator.find(m_date)->second.toString());
}

void CalcDateRsiTask::setParam(const IntDateTime& date,
	const std::shared_ptr<StockMarket>& spMarket,
	std::map<std::string, std::vector<std::vector<std::string>>>* indicatorData)
{
	m_date = date;
	m_spMarket = spMarket;
	m_indicatorData = indicatorData;
}