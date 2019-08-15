#include "StockWrIndicator.h"
#include "StockWr.h"

StockWrIndicator::StockWrIndicator()
{

}

void StockWrIndicator::setRedisData(const std::string& stock, const std::vector<std::vector<std::string>>& vecIndicator)
{
	m_stock = stock;
	m_vecRedisIndicator = vecIndicator;
}

void StockWrIndicator::load()
{
	clear();
	int32_t index = -1;
	while (index++ != m_vecRedisIndicator.size() - 1)
	{
		std::shared_ptr<StockWr> spStockWr(new StockWr);
		spStockWr->m_date = m_vecRedisIndicator[index][0];
		spStockWr->m_wr10 = m_vecRedisIndicator[index][1].c_str();
		spStockWr->m_wr20 = m_vecRedisIndicator[index][2].c_str();
		m_indicator[spStockWr->m_date] = spStockWr;
	}
	calc();
	m_vecRedisIndicator.clear();
}

std::shared_ptr<StockWr> StockWrIndicator::day(const IntDateTime& date)
{
	auto itDate = m_indicator.find(date);
	if (itDate == m_indicator.end())
	{
		return std::shared_ptr<StockWr>();
	}
	return itDate->second;
}

void StockWrIndicator::clear()
{
	m_indicator.clear();
}

bool StockWrIndicator::empty()
{
	return m_indicator.empty();
}

void StockWrIndicator::calc()
{

}
