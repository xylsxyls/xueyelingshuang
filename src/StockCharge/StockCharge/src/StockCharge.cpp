#include "StockCharge.h"

StockCharge::StockCharge()
{
	m_rate = 1.6;
	m_rate = m_rate  / 10000.0;
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
	return transferFee(price, position) + commission(price, position);
}

BigNumber StockCharge::sellFee(const std::string& stock, const BigNumber& price, const BigNumber& position)
{
	return transferFee(price, position) + commission(price, position) + stampDuty(price, position);
}

BigNumber StockCharge::stampDuty(const BigNumber& price, const BigNumber& position)
{
	return (price * position / 2000.0).toPrec(2);
}

BigNumber StockCharge::transferFee(const BigNumber& price, const BigNumber& position)
{
	return (price * position / 100000.0).toPrec(2);
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

BigNumber StockCharge::brokerage(const BigNumber& price, const BigNumber& position)
{
	return (price * position * 0.341 / 10000.0).toPrec(2);
}

BigNumber StockCharge::Custody(const BigNumber& price, const BigNumber& position)
{
	return (price * position / 50000.0).toPrec(2);
}
