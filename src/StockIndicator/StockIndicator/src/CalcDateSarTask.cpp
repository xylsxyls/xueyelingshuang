#include "CalcDateSarTask.h"
#include "StockIndicatorHelper.h"
#include "StockMarket/StockMarketAPI.h"

CalcDateSarTask::CalcDateSarTask() :
m_indicatorData(nullptr)
{
	m_spMarket = nullptr;
}

void CalcDateSarTask::DoTask()
{
	m_spMarket->load();
	if (!m_spMarket->setDate(m_date))
	{
		return;
	}
	std::map<std::string, std::vector<std::vector<std::string>>>& indicatorData = *m_indicatorData;
	std::string stock = m_spMarket->stock();
	indicatorData[stock];

	std::map<IntDateTime, std::pair<BigNumber, int32_t>> sar5Indicator = StockIndicatorHelper::sar(*m_spMarket, 5);
	std::map<IntDateTime, std::pair<BigNumber, int32_t>> sar10Indicator = StockIndicatorHelper::sar(*m_spMarket, 10);
	std::map<IntDateTime, std::pair<BigNumber, int32_t>> sar20Indicator = StockIndicatorHelper::sar(*m_spMarket, 20);

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
	const std::shared_ptr<StockMarket>& spMarket,
	std::map<std::string, std::vector<std::vector<std::string>>>* indicatorData)
{
	m_date = date;
	m_spMarket = spMarket;
	m_indicatorData = indicatorData;
}