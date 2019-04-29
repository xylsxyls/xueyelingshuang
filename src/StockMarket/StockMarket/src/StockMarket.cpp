#include "StockMarket.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"

bool StockMarket::load(const std::string& stock)
{
	Ctxt txt("D:\\Table.txt");
	txt.LoadTxt(Ctxt::SPLIT, "\t\t");

	int32_t lineIndex = 0;
	while (lineIndex++ != txt.m_vectxt.size() - 1)
	{
		std::vector<std::string> vec = CStringManager::split(txt.m_vectxt[lineIndex][0], ",");
		if (vec[0].empty())
		{
			break;
		}
		IntDateTime date = vec[0];
		std::vector<std::string> price = CStringManager::split(vec[1], "\t");
		std::vector<BigNumber> data;
		data.push_back(BigNumber(price[1].c_str()));
		data.push_back(BigNumber(txt.m_vectxt[lineIndex][1].c_str()));
		data.push_back(BigNumber(txt.m_vectxt[lineIndex][2].c_str()));
		data.push_back(BigNumber(txt.m_vectxt[lineIndex][3].c_str()));
		m_histroy[date] = data;
	}
	m_stock = stock;
	return true;
}

void StockMarket::add(const IntDateTime& date, const std::vector<BigNumber>& data)
{
	m_histroy[date] = data;
}

std::string StockMarket::stock() const
{
	return m_stock;
}

std::string StockMarket::name() const
{
	return m_name;
}

BigNumber StockMarket::open(const IntDateTime& date) const
{
	if (!dateExist(date))
	{
		return -1;
	}
	return m_histroy.find(date)->second[OPEN];
}

BigNumber StockMarket::high(const IntDateTime& date) const
{
	if (!dateExist(date))
	{
		return -1;
	}
	return m_histroy.find(date)->second[HIGH];
}

BigNumber StockMarket::low(const IntDateTime& date) const
{
	if (!dateExist(date))
	{
		return -1;
	}
	return m_histroy.find(date)->second[LOW];
}

BigNumber StockMarket::close(const IntDateTime& date) const
{
	if (!dateExist(date))
	{
		return -1;
	}
	return m_histroy.find(date)->second[CLOSE];
}

BigNumber StockMarket::preOpen(const IntDateTime& date) const
{
	if (!dateExist(date))
	{
		return -1;
	}
	IntDateTime preDate;
	if (!getDatePre(date, preDate))
	{
		return -1;
	}
	return m_histroy.find(preDate)->second[OPEN];
}

BigNumber StockMarket::preHigh(const IntDateTime& date) const
{
	if (!dateExist(date))
	{
		return -1;
	}
	IntDateTime preDate;
	if (!getDatePre(date, preDate))
	{
		return -1;
	}
	return m_histroy.find(preDate)->second[HIGH];
}

BigNumber StockMarket::preLow(const IntDateTime& date) const
{
	if (!dateExist(date))
	{
		return -1;
	}
	IntDateTime preDate;
	if (!getDatePre(date, preDate))
	{
		return -1;
	}
	return m_histroy.find(preDate)->second[LOW];
}

BigNumber StockMarket::preClose(const IntDateTime& date) const
{
	if (!dateExist(date))
	{
		return -1;
	}
	IntDateTime preDate;
	if (!getDatePre(date, preDate))
	{
		return -1;
	}
	return m_histroy.find(preDate)->second[CLOSE];
}

BigNumber StockMarket::nextOpen(const IntDateTime& date) const
{
	if (!dateExist(date))
	{
		return -1;
	}
	IntDateTime nextDate;
	if (!getDateNext(date, nextDate))
	{
		return -1;
	}
	return m_histroy.find(nextDate)->second[OPEN];
}

BigNumber StockMarket::nextHigh(const IntDateTime& date) const
{
	if (!dateExist(date))
	{
		return -1;
	}
	IntDateTime nextDate;
	if (!getDateNext(date, nextDate))
	{
		return -1;
	}
	return m_histroy.find(nextDate)->second[HIGH];
}

BigNumber StockMarket::nextLow(const IntDateTime& date) const
{
	if (!dateExist(date))
	{
		return -1;
	}
	IntDateTime nextDate;
	if (!getDateNext(date, nextDate))
	{
		return -1;
	}
	return m_histroy.find(nextDate)->second[LOW];
}

BigNumber StockMarket::nextClose(const IntDateTime& date) const
{
	if (!dateExist(date))
	{
		return -1;
	}
	IntDateTime nextDate;
	if (!getDateNext(date, nextDate))
	{
		return -1;
	}
	return m_histroy.find(nextDate)->second[CLOSE];
}

bool StockMarket::dateExist(const IntDateTime& date) const
{
	return m_histroy.find(date) != m_histroy.end();
}

bool StockMarket::getDatePre(const IntDateTime& date, IntDateTime& preDate) const
{
	if (!dateExist(date))
	{
		return false;
	}
	auto itDate = m_histroy.find(date);
	if (itDate == m_histroy.begin())
	{
		return false;
	}
	preDate = (--itDate)->first;
	return true;
}

bool StockMarket::getDateNext(const IntDateTime& date, IntDateTime& nextDate) const
{
	if (!dateExist(date))
	{
		return false;
	}
	auto itDate = ++(m_histroy.find(date));
	if (itDate == m_histroy.end())
	{
		return false;
	}
	nextDate = itDate->first;
	return true;
}

bool StockMarket::getMarketPre(const IntDateTime& date, IntDateTime& preDate, std::vector<BigNumber>& preData) const
{
	if (!dateExist(date))
	{
		return false;
	}
	if (!getDatePre(date, preDate))
	{
		return false;
	}
	preData = m_histroy.find(preDate)->second;
	return true;
}

bool StockMarket::getMarketNext(const IntDateTime& date, IntDateTime& nextDate, std::vector<BigNumber>& nextData) const
{
	if (!dateExist(date))
	{
		return false;
	}
	if (!getDateNext(date, nextDate))
	{
		return false;
	}
	nextData = m_histroy.find(nextDate)->second;
	return true;
}

bool StockMarket::getMarketPre(const IntDateTime& date, int32_t days, std::map<IntDateTime, std::vector<BigNumber>>& preDaysData) const
{
	if (!dateExist(date))
	{
		return false;
	}

	IntDateTime preDate = date;
	int32_t dataDays = days;
	--days;
	while (days-- != 0)
	{
		if (!getDatePre(preDate, preDate))
		{
			return false;
		}
	}
	preDaysData.clear();
	while (dataDays-- != 0)
	{
		preDaysData[preDate] = m_histroy.find(preDate)->second;
		getDateNext(preDate, preDate);
	}
	return true;
}

bool StockMarket::getMarketNext(const IntDateTime& date, int32_t days, std::map<IntDateTime, std::vector<BigNumber>>& nextDaysData) const
{
	if (!dateExist(date))
	{
		return false;
	}
	IntDateTime nextDate = date;
	int32_t dataDays = days;
	while (days-- != 0)
	{
		if (!getDateNext(nextDate, nextDate))
		{
			return false;
		}
	}
	nextDaysData.clear();
	nextDate = date;
	while (dataDays-- != 0)
	{
		getDateNext(nextDate, nextDate);
		nextDaysData[nextDate] = m_histroy.find(nextDate)->second;
	}
	return true;
}