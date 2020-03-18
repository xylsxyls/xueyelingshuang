#include "StockDay.h"

void StockDay::load(const std::string& stock,
	const IntDateTime& date,
	const BigNumber& open,
	const BigNumber& high,
	const BigNumber& low,
	const BigNumber& close,
	const BigNumber& preClose)
{
	m_stock = stock;
	m_date = date;
	m_open = open;
	m_high = high;
	m_low = low;
	m_close = close;
	m_preClose = preClose;
}

std::string StockDay::stock()
{
	return m_stock;
}

IntDateTime StockDay::date()
{
	return m_date;
}

BigNumber StockDay::open() const
{
	return m_open;
}

BigNumber StockDay::high() const
{
	return m_high;
}

BigNumber StockDay::low() const
{
	return m_low;
}

BigNumber StockDay::close() const
{
	return m_close;
}

BigNumber StockDay::preClose() const
{
	return m_preClose;
}

BigNumber StockDay::upValue() const
{
	return m_high - BigNumber::maxNumber(m_open, m_close);
}

BigNumber StockDay::downValue() const
{
	return BigNumber::minNumber(m_open, m_close) - m_low;
}

BigNumber StockDay::entityValue() const
{
	return (m_open - m_close).abs();
}

BigNumber StockDay::riseFallValue() const
{
	return m_close - m_preClose;
}

BigNumber StockDay::chgValue() const
{
	return (((m_close / m_preClose.toPrec(6).zero()) - 1) * 100).toPrec(2);
}

BigNumber StockDay::openChgValue() const
{
	return (((m_open / m_preClose.toPrec(6).zero()) - 1) * 100).toPrec(2);
}

BigNumber StockDay::highChgValue() const
{
	return (((m_high / m_preClose.toPrec(6).zero()) - 1) * 100).toPrec(2);
}

BigNumber StockDay::lowChgValue() const
{
	return (((m_low / m_preClose.toPrec(6).zero()) - 1) * 100).toPrec(2);
}

BigNumber StockDay::fourAvgChgValue() const
{
	return ((openChgValue() + highChgValue() + lowChgValue() + chgValue()) / 4).toPrec(2);
}

BigNumber StockDay::amplitudeValue() const
{
	return ((m_high - m_low) / m_preClose.toPrec(6).zero() * 100).toPrec(2);
}

bool StockDay::isLimitUp() const
{
	return (m_preClose.toPrec(6) * 1.1).toPrec(2) <= m_close;
}

bool StockDay::isLimitDown() const
{
	return (m_preClose.toPrec(6) * 0.9).toPrec(2) >= m_close;
}