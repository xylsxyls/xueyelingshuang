#include "StockSarIndicator.h"
#include "StockSar.h"

void StockSarIndicator::load()
{
	clear();
	int32_t index = -1;
	while (index++ != m_vecRedisIndicator->size() - 1)
	{
		std::shared_ptr<StockSar> spStockSar(new StockSar);
		spStockSar->m_date = (*m_vecRedisIndicator)[index][m_dateIndex];
		spStockSar->m_sar = (*m_vecRedisIndicator)[index][m_indicatorIndex[0]].c_str();
		m_indicator[spStockSar->m_date] = spStockSar;
	}
	calc();
	m_vecRedisIndicator = nullptr;
}

void StockSarIndicator::calc()
{

}