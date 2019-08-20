#ifndef _INDICATOR_MANAGER_BASE_INL__
#define _INDICATOR_MANAGER_BASE_INL__

#include "IndicatorManagerBase.h"

template <class StockType>
IndicatorManagerBase<StockType>::IndicatorManagerBase()
{

}

template <class StockType>
void IndicatorManagerBase<StockType>::setRedisData(const std::string& stock,
	const std::shared_ptr<std::vector<std::vector<std::string>>>& vecIndicator,
	int32_t dateIndex,
	const std::vector<int32_t>& indicatorIndex)
{
	m_stock = stock;
	m_vecRedisIndicator = vecIndicator;
	m_dateIndex = dateIndex;
	m_indicatorIndex = indicatorIndex;
}

template <class StockType>
std::shared_ptr<StockType> IndicatorManagerBase<StockType>::day(const IntDateTime& date)
{
	auto itDate = m_indicator.find(date);
	if (itDate == m_indicator.end())
	{
		return std::shared_ptr<StockType>();
	}
	return itDate->second;
}

template <class StockType>
void IndicatorManagerBase<StockType>::clear()
{
	m_indicator.clear();
}

template <class StockType>
bool IndicatorManagerBase<StockType>::empty()
{
	return m_indicator.empty();
}

#endif