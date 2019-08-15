#include "StockRsiIndicator.h"
#include "StockRsi.h"

StockRsiIndicator::StockRsiIndicator()
{

}

void StockRsiIndicator::setRedisData(const std::string& stock, const std::vector<std::vector<std::string>>& vecIndicator)
{
	m_stock = stock;
	m_vecRedisIndicator = vecIndicator;
}

void StockRsiIndicator::load(const std::string& stock, const IntDateTime& beginTime, const IntDateTime& endTime)
{
	clear();
	m_stock = stock;
	int32_t index = -1;
	while (index++ != m_vecRedisIndicator.size() - 1)
	{
		std::shared_ptr<StockRsi> spStockRsi(new StockRsi);
		spStockRsi->m_date = m_vecRedisIndicator[index][0];
		spStockRsi->m_rsi6 = m_vecRedisIndicator[index][1].c_str();
		spStockRsi->m_rsi12 = m_vecRedisIndicator[index][2].c_str();
		spStockRsi->m_rsi24 = m_vecRedisIndicator[index][3].c_str();
		m_indicator[spStockRsi->m_date] = spStockRsi;
	}
	calc();
}

std::shared_ptr<StockRsi> StockRsiIndicator::day(const IntDateTime& date)
{
	auto itDate = m_indicator.find(date);
	if (itDate == m_indicator.end())
	{
		return std::shared_ptr<StockRsi>();
	}
	return itDate->second;
}

void StockRsiIndicator::clear()
{
	m_indicator.clear();
}

bool StockRsiIndicator::empty()
{
	return m_indicator.empty();
}

void StockRsiIndicator::calc()
{

}
