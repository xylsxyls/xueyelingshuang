#include "StockRsiIndicator.h"
#include "StockRsi.h"

void StockRsiIndicator::load()
{
	clear();
	int32_t index = -1;
	while (index++ != m_vecRedisIndicator->size() - 1)
	{
		std::shared_ptr<StockRsi> spStockRsi(new StockRsi);
		spStockRsi->m_date = (*m_vecRedisIndicator)[index][m_dateIndex];
		spStockRsi->m_rsi6 = (*m_vecRedisIndicator)[index][m_indicatorIndex[0]].c_str();
		spStockRsi->m_rsi12 = (*m_vecRedisIndicator)[index][m_indicatorIndex[1]].c_str();
		spStockRsi->m_rsi24 = (*m_vecRedisIndicator)[index][m_indicatorIndex[2]].c_str();
		m_indicator[spStockRsi->m_date] = spStockRsi;
	}
	calc();
	m_vecRedisIndicator = nullptr;
}

void StockRsiIndicator::calc()
{

}