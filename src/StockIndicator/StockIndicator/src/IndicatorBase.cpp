#include "IndicatorBase.h"

std::string IndicatorBase::stock()
{
	return m_stock;
}

IntDateTime IndicatorBase::date()
{
	return m_date;
}

bool IndicatorBase::needBuy()
{
	return m_needBuy;
}

bool IndicatorBase::needSell()
{
	return m_needSell;
}

BigNumber IndicatorBase::buyPercent()
{
	return m_buyPercent;
}

BigNumber IndicatorBase::sellPercent()
{
	return m_sellPercent;
}

BigNumber IndicatorBase::buyScore()
{
	return m_buyScore;
}

BigNumber IndicatorBase::sellScore()
{
	return m_sellScore;
}