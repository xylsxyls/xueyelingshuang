#include "StockWrIndicator.h"
#include "StockWr.h"
#include "StockMysql/StockMysqlAPI.h"

void StockWrIndicator::load(const std::string& stock, const IntDateTime& beginTime, const IntDateTime& endTime)
{
	clear();
	m_stock = stock;
	std::vector<std::vector<std::string>> vecIndicator = StockMysql::instance().readWr(stock, beginTime, endTime);
	int32_t index = -1;
	while (index++ != vecIndicator.size() - 1)
	{
		std::shared_ptr<StockWr> spStockWr(new StockWr);
		spStockWr->m_date = vecIndicator[index][0];
		spStockWr->m_wr10 = vecIndicator[index][1].c_str();
		spStockWr->m_wr20 = vecIndicator[index][2].c_str();
		m_indicator[spStockWr->m_date] = spStockWr;
	}
	calc();
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
	m_stock.clear();
	m_indicator.clear();
}

bool StockWrIndicator::empty()
{
	return m_indicator.empty();
}

void StockWrIndicator::calc()
{

}
