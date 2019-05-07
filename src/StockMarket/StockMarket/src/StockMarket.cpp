#include "StockMarket.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"

void StockMarket::load(const std::string& stock, const std::string& name, const std::map<IntDateTime, std::vector<BigNumber>>& history)
{
	m_stock = stock;
	m_name = name;
	m_history = history;
}

void StockMarket::save(const std::string& stock, const MysqlCpp& mysql, const std::string& file)
{
	mysql.selectDb("stockmarket");
	std::vector<std::string> vecFields;
	vecFields.push_back("date varchar(10)");
	vecFields.push_back("open varchar(7)");
	vecFields.push_back("high varchar(7)");
	vecFields.push_back("low varchar(7)");
	vecFields.push_back("close varchar(7)");
	mysql.execute(mysql.PreparedStatementCreator(SqlString::createTableString(stock, vecFields)));
	Ctxt txt(file);
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
		std::string& open = price[1];
		std::string& high = txt.m_vectxt[lineIndex][1];
		std::string& low = txt.m_vectxt[lineIndex][2];
		std::string& close = txt.m_vectxt[lineIndex][3];
		auto prepare = mysql.PreparedStatementCreator(SqlString::insertString(stock, "date,open,high,low,close"));
		prepare->setString(0, date.dateToString());
		prepare->setString(1, open);
		prepare->setString(2, high);
		prepare->setString(3, low);
		prepare->setString(4, close);
		mysql.execute(prepare);
	}

	auto prepare = mysql.PreparedStatementCreator(SqlString::insertString("stock", "stock,name"));
	prepare->setString(0, stock);
	prepare->setString(1, "");
	mysql.execute(prepare);
}

void StockMarket::add(const IntDateTime& date, const std::vector<BigNumber>& data)
{
	m_history[date] = data;
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
	return m_history.find(date)->second[OPEN];
}

BigNumber StockMarket::high(const IntDateTime& date) const
{
	if (!dateExist(date))
	{
		return -1;
	}
	return m_history.find(date)->second[HIGH];
}

BigNumber StockMarket::low(const IntDateTime& date) const
{
	if (!dateExist(date))
	{
		return -1;
	}
	return m_history.find(date)->second[LOW];
}

BigNumber StockMarket::close(const IntDateTime& date) const
{
	if (!dateExist(date))
	{
		return -1;
	}
	return m_history.find(date)->second[CLOSE];
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
	return m_history.find(preDate)->second[OPEN];
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
	return m_history.find(preDate)->second[HIGH];
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
	return m_history.find(preDate)->second[LOW];
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
	return m_history.find(preDate)->second[CLOSE];
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
	return m_history.find(nextDate)->second[OPEN];
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
	return m_history.find(nextDate)->second[HIGH];
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
	return m_history.find(nextDate)->second[LOW];
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
	return m_history.find(nextDate)->second[CLOSE];
}

BigNumber StockMarket::upValue(const IntDateTime& date) const
{
	BigNumber openValue = open(date);
	BigNumber closeValue = close(date);
	if (openValue < closeValue)
	{
		return high(date) - close(date);
	}
	return high(date) - open(date);
}

BigNumber StockMarket::downValue(const IntDateTime& date) const
{
	BigNumber openValue = open(date);
	BigNumber closeValue = close(date);
	if (openValue < closeValue)
	{
		return open(date) - low(date);
	}
	return close(date) - low(date);
}

BigNumber StockMarket::entityValue(const IntDateTime& date) const
{
	return (open(date) - close(date)).abs();
}

BigNumber StockMarket::chgValue(const IntDateTime& date) const
{
	return (((close(date) / preClose(date).toPrec(6)) - 1) * 100).toPrec(2);
}

bool StockMarket::isLimitUp(const IntDateTime& date) const
{
	return (preClose(date).toPrec(6) * 1.1).toPrec(2) == close(date);
}

bool StockMarket::isLimitDown(const IntDateTime& date) const
{
	return (preClose(date).toPrec(6) * 0.9).toPrec(2) == close(date);
}

bool StockMarket::dateExist(const IntDateTime& date) const
{
	return m_history.find(date) != m_history.end();
}

bool StockMarket::getDatePre(const IntDateTime& date, IntDateTime& preDate) const
{
	if (!dateExist(date))
	{
		return false;
	}
	auto itDate = m_history.find(date);
	if (itDate == m_history.begin())
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
	auto itDate = ++(m_history.find(date));
	if (itDate == m_history.end())
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
	preData = m_history.find(preDate)->second;
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
	nextData = m_history.find(nextDate)->second;
	return true;
}

std::map<IntDateTime, std::vector<BigNumber>> StockMarket::history() const
{
	return m_history;
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
		preDaysData[preDate] = m_history.find(preDate)->second;
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
		nextDaysData[nextDate] = m_history.find(nextDate)->second;
	}
	return true;
}