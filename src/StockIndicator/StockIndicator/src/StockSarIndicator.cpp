#include "StockSarIndicator.h"
#include "StockSar.h"

StockSarIndicator::StockSarIndicator()
{
	m_type = "sar";
}

void StockSarIndicator::load()
{
	clear();
	int32_t index = -1;
	while (index++ != m_vecRedisIndicator->size() - 1)
	{
		std::shared_ptr<StockSar> spStockSar(new StockSar);
		spStockSar->m_date = (*m_vecRedisIndicator)[index][m_dateIndex];
		spStockSar->m_sar5 = (*m_vecRedisIndicator)[index][m_indicatorIndex[0]].c_str();
		spStockSar->m_sarState5 = (StockSar::SarState)atoi((*m_vecRedisIndicator)[index][m_indicatorIndex[1]].c_str());
		spStockSar->m_sar10 = (*m_vecRedisIndicator)[index][m_indicatorIndex[2]].c_str();
		spStockSar->m_sarState10 = (StockSar::SarState)atoi((*m_vecRedisIndicator)[index][m_indicatorIndex[3]].c_str());
		spStockSar->m_sar20 = (*m_vecRedisIndicator)[index][m_indicatorIndex[4]].c_str();
		spStockSar->m_sarState20 = (StockSar::SarState)atoi((*m_vecRedisIndicator)[index][m_indicatorIndex[5]].c_str());
		m_indicator[spStockSar->m_date] = spStockSar;
	}
	calc();
	m_vecRedisIndicator = nullptr;
}

void StockSarIndicator::calc()
{

}

std::shared_ptr<StockSar> StockSarIndicator::day(const IntDateTime& date)
{
	return std::dynamic_pointer_cast<StockSar>(IndicatorManagerBase::day(date));
}