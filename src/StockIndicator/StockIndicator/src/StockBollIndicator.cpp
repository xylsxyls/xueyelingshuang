#include "StockBollIndicator.h"
#include "StockBoll.h"

StockBollIndicator::StockBollIndicator()
{
	m_type = "boll";
}

void StockBollIndicator::load()
{
	clear();
	int32_t index = -1;
	while (index++ != m_vecRedisIndicator->size() - 1)
	{
		std::shared_ptr<StockBoll> spStockBoll(new StockBoll);
		spStockBoll->m_date = (*m_vecRedisIndicator)[index][m_dateIndex];
		spStockBoll->m_mid = (*m_vecRedisIndicator)[index][m_indicatorIndex[0]].c_str();
		spStockBoll->m_up = (*m_vecRedisIndicator)[index][m_indicatorIndex[1]].c_str();
		spStockBoll->m_down = (*m_vecRedisIndicator)[index][m_indicatorIndex[2]].c_str();
		m_indicator[spStockBoll->m_date] = spStockBoll;
	}
	calc();
	m_vecRedisIndicator = nullptr;
}

void StockBollIndicator::calc()
{

}

std::shared_ptr<StockBoll> StockBollIndicator::day(const IntDateTime& date)
{
	return std::dynamic_pointer_cast<StockBoll>(IndicatorManagerBase::day(date));
}