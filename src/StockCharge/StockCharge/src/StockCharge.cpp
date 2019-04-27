#include "StockCharge.h"

StockCharge::StockCharge()
{
	m_rate = 25;
	m_rate = m_rate / 100000.0;
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

BigNumber StockCharge::buyFee(const std::string& stock, const BigNumber& price, const BigNumber& position)
{
	return (stock[0] == '6' ? transferFee(price, position) : 0) + commission(price, position);
}

BigNumber StockCharge::sellFee(const std::string& stock, const BigNumber& price, const BigNumber& position)
{
	return (stock[0] == '6' ? transferFee(price, position) : 0) + commission(price, position) + stampDuty(price, position);
}

BigNumber StockCharge::stampDuty(const BigNumber& price, const BigNumber& position)
{
	return (price * position / 1000.0).toPrec(2);
}

BigNumber StockCharge::transferFee(const BigNumber& price, const BigNumber& position)
{
	return (price * position * 2 / 100000.0).toPrec(2);
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