#include "IndicatorManagerBase.h"
#include "IndicatorBase.h"

IndicatorManagerBase::IndicatorManagerBase()
{

}

IndicatorManagerBase::~IndicatorManagerBase()
{

}

void IndicatorManagerBase::setRedisData(const std::string& stock,
	const std::shared_ptr<std::vector<std::vector<std::string>>>& vecIndicator,
	int32_t dateIndex,
	const std::vector<int32_t>& indicatorIndex)
{
	m_stock = stock;
	m_vecRedisIndicator = vecIndicator;
	m_dateIndex = dateIndex;
	m_indicatorIndex = indicatorIndex;
}

std::shared_ptr<IndicatorBase> IndicatorManagerBase::day(const IntDateTime& date)
{
	auto itDate = m_indicator.find(date);
	if (itDate == m_indicator.end())
	{
		return std::shared_ptr<IndicatorBase>();
	}
	return itDate->second;
}

void IndicatorManagerBase::clear()
{
	m_indicator.clear();
}

bool IndicatorManagerBase::empty()
{
	return m_indicator.empty();
}

std::string IndicatorManagerBase::type()
{
	return m_type;
}