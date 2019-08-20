#include "StockAvgIndicator.h"
#include "StockAvg.h"

void StockAvgIndicator::load()
{
	clear();
	
	int32_t index = -1;
	while (index++ != m_vecRedisIndicator->size() - 1)
	{
		std::shared_ptr<StockAvg> spStockAvg(new StockAvg);
		spStockAvg->m_date = (*m_vecRedisIndicator)[index][m_dateIndex];
		spStockAvg->m_avg9_30 = (*m_vecRedisIndicator)[index][m_indicatorIndex[0]].c_str();
		spStockAvg->m_avg10_00 = (*m_vecRedisIndicator)[index][m_indicatorIndex[1]].c_str();
		spStockAvg->m_avg10_30 = (*m_vecRedisIndicator)[index][m_indicatorIndex[2]].c_str();
		spStockAvg->m_avg11_00 = (*m_vecRedisIndicator)[index][m_indicatorIndex[3]].c_str();
		spStockAvg->m_avg11_30 = (*m_vecRedisIndicator)[index][m_indicatorIndex[4]].c_str();
		spStockAvg->m_avg13_30 = (*m_vecRedisIndicator)[index][m_indicatorIndex[5]].c_str();
		spStockAvg->m_avg14_00 = (*m_vecRedisIndicator)[index][m_indicatorIndex[6]].c_str();
		spStockAvg->m_avg14_30 = (*m_vecRedisIndicator)[index][m_indicatorIndex[7]].c_str();
		spStockAvg->m_avg15_00 = (*m_vecRedisIndicator)[index][m_indicatorIndex[8]].c_str();
		spStockAvg->m_avgHigh = (*m_vecRedisIndicator)[index][m_indicatorIndex[9]].c_str();
		spStockAvg->m_avgLow = (*m_vecRedisIndicator)[index][m_indicatorIndex[10]].c_str();
		m_indicator[spStockAvg->m_date] = spStockAvg;
	}
	calc();
	m_vecRedisIndicator = nullptr;
}

void StockAvgIndicator::calc()
{

}