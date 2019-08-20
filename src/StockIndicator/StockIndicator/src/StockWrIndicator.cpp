#include "StockWrIndicator.h"
#include "StockWr.h"

void StockWrIndicator::load()
{
	clear();
	
	int32_t index = -1;
	while (index++ != m_vecRedisIndicator->size() - 1)
	{
		std::shared_ptr<StockWr> spStockWr(new StockWr);
		spStockWr->m_date = (*m_vecRedisIndicator)[index][m_dateIndex];
		spStockWr->m_wr10 = (*m_vecRedisIndicator)[index][m_indicatorIndex[0]].c_str();
		spStockWr->m_wr20 = (*m_vecRedisIndicator)[index][m_indicatorIndex[1]].c_str();
		m_indicator[spStockWr->m_date] = spStockWr;
	}
	calc();
	m_vecRedisIndicator = nullptr;
}

void StockWrIndicator::calc()
{

}