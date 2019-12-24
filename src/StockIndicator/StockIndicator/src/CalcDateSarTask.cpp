#include "CalcDateSarTask.h"
#include "StockIndicatorHelper.h"
#include "StockMarket/StockMarketAPI.h"

CalcDateSarTask::CalcDateSarTask() :
m_indicatorData(nullptr)
{

}

void CalcDateSarTask::DoTask()
{
	if (!m_market.setDate(m_date))
	{
		return;
	}
	std::map<std::string, std::vector<std::vector<std::string>>>& indicatorData = *m_indicatorData;
	std::string stock = m_market.stock();
	indicatorData[stock];

	std::map<IntDateTime, std::pair<BigNumber, int32_t>> sar5Indicator = StockIndicatorHelper::sar(m_market, 5);
	std::map<IntDateTime, std::pair<BigNumber, int32_t>> sar10Indicator = StockIndicatorHelper::sar(m_market, 10);
	std::map<IntDateTime, std::pair<BigNumber, int32_t>> sar20Indicator = StockIndicatorHelper::sar(m_market, 20);

	auto& stockData = indicatorData.find(stock)->second;
	stockData.push_back(std::vector<std::string>());
	std::vector<std::string>& vecLine = stockData.back();
	vecLine.push_back(m_date.dateToString());
	vecLine.push_back(sar5Indicator.find(m_date)->second.first.toString());
	vecLine.push_back(std::to_string(sar5Indicator.find(m_date)->second.second));
	vecLine.push_back(sar10Indicator.find(m_date)->second.first.toString());
	vecLine.push_back(std::to_string(sar10Indicator.find(m_date)->second.second));
	vecLine.push_back(sar20Indicator.find(m_date)->second.first.toString());
	vecLine.push_back(std::to_string(sar20Indicator.find(m_date)->second.second));
}

void CalcDateSarTask::setParam(const IntDateTime& date,
	const StockMarket& market,
	std::map<std::string, std::vector<std::vector<std::string>>>* indicatorData)
{
	m_date = date;
	m_market = market;
	m_indicatorData = indicatorData;
}