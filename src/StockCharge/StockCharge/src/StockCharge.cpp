#include "StockCharge.h"

StockCharge::StockCharge()
{
	m_rate = 25;
	m_rate = m_rate / 100000;
	m_minimum = 5;
}

StockCharge& StockCharge::instance()
{
	static StockCharge s_stockCharge;
	return s_stockCharge;
}

void StockCharge::init(const BigNumber& rate, const BigNumber& minimum)
{
	m_rate = rate;
	m_minimum = minimum;
}

BigNumber StockCharge::buyFee(const BigNumber& price, const BigNumber& position)
{
	return transferFee(price, position) + commission(price, position);
}

BigNumber StockCharge::sellFee(const BigNumber& price, const BigNumber& position)
{
	return stampDuty(price, position) + commission(price, position);
}

BigNumber StockCharge::stampDuty(const BigNumber& price, const BigNumber& position)
{
	return (price * position / 1000).toPrec(2);
}

BigNumber StockCharge::transferFee(const BigNumber& price, const BigNumber& position)
{
	return (price * position * 2 / 100000).toPrec(2);
}

BigNumber StockCharge::commission(const BigNumber& price, const BigNumber& position)
{
	BigNumber commissionFee = price * position * m_rate;
	if (commissionFee < m_minimum)
	{
		return m_minimum;
	}
	return commissionFee.toPrec(2);
}