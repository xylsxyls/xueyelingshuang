#include "StockMarket.h"
#include "CStringManager/CStringManagerAPI.h"
#include "StockMysql/StockMysqlAPI.h"
#include "StockDay.h"

StockMarket::StockMarket()
{
	m_spStockMysql = StockMysql::newCase();
}

void StockMarket::load(const std::string& stock,
	const IntDateTime& beginTime,
	const IntDateTime& endTime)
{
	clear();
	m_stock = stock;
	std::vector<std::vector<std::string>> vecMarket = m_spStockMysql->readMarket(stock, beginTime, endTime);
	if (vecMarket.empty())
	{
		return;
	}
	
	IntDateTime preDate = m_spStockMysql->getDatePre(stock, beginTime);
	std::shared_ptr<StockDay> spFirstDay(new StockDay);
	m_date = vecMarket[0][0];
	spFirstDay->load(stock,
		vecMarket[0][0],
		vecMarket[0][1].c_str(),
		vecMarket[0][2].c_str(),
		vecMarket[0][3].c_str(),
		vecMarket[0][4].c_str(),
		preDate.empty() ? (BigNumber(vecMarket[0][1].c_str()) / 1.1).toPrec(2) : m_spStockMysql->readMarket(stock, preDate, preDate)[0][4].c_str());
	m_history[vecMarket[0][0]] = spFirstDay;

	int32_t dayIndex = 0;
	while (dayIndex++ != vecMarket.size() - 1)
	{
		std::shared_ptr<StockDay> spDay(new StockDay);
		spDay->load(stock,
			vecMarket[dayIndex][0],
			vecMarket[dayIndex][1].c_str(),
			vecMarket[dayIndex][2].c_str(),
			vecMarket[dayIndex][3].c_str(),
			vecMarket[dayIndex][4].c_str(),
			vecMarket[dayIndex - 1][4].c_str());
		m_history[vecMarket[dayIndex][0]] = spDay;
	}
}

std::string StockMarket::stock() const
{
	return m_stock;
}

std::string StockMarket::name() const
{
	return m_name;
}

void StockMarket::clear()
{
	m_stock.clear();
	m_name.clear();
	m_date.clear();
	m_history.clear();
}

std::shared_ptr<StockDay> StockMarket::day() const
{
	auto itDate = m_history.find(m_date);
	if (itDate == m_history.end())
	{
		return std::shared_ptr<StockDay>();
	}
	return itDate->second;
}

IntDateTime StockMarket::date() const
{
	return m_date;
}

void StockMarket::setDate(const IntDateTime& date)
{
	if (m_history.find(date) == m_history.end())
	{
		return;
	}
	m_date = date;
}

void StockMarket::setFirstDate()
{
	if (m_history.empty())
	{
		return;
	}
	m_date = m_history.begin()->first;
}

void StockMarket::setEndDate()
{
	if (m_history.empty())
	{
		return;
	}
	m_date = m_history.rbegin()->first;
}

bool StockMarket::previous()
{
	if (m_history.empty())
	{
		return false;
	}
	auto itDate = m_history.find(m_date);
	if (itDate == m_history.end())
	{
		return false;
	}
	if (itDate == m_history.begin())
	{
		return false;
	}
	m_date = (--itDate)->first;
	return true;
}

bool StockMarket::next()
{
	if (m_history.empty())
	{
		return false;
	}
	auto itDate = m_history.find(m_date);
	if (itDate == m_history.end())
	{
		return false;
	}
	if ((++itDate) == m_history.end())
	{
		return false;
	}
	m_date = itDate->first;
	return true;
}

void StockMarket::setStock(const std::string& stock)
{
	m_stock = stock;
}

std::shared_ptr<StockDay> StockMarket::stockDay(const IntDateTime& date) const
{
	StockMarket day;
	day.load(m_stock, date, date);
	return day.day();
}

//std::shared_ptr<StockDay> StockMarket::stockPreDay(const IntDateTime& date)
//{
//	IntDateTime preDate = StockMysql::instance().getDatePre(m_stock, date);
//	if (preDate.empty())
//	{
//		return std::shared_ptr<StockDay>();
//	}
//	StockMarket day;
//	day.load(m_stock, preDate, preDate);
//	return day.stockDay(preDate);
//}
//
//std::shared_ptr<StockDay> StockMarket::stockNextDay(const IntDateTime& date, bool useMysql)
//{
//	if (useMysql)
//	{
//		IntDateTime nextDate = StockMysql::instance().getDateNext(m_stock, date);
//		if (nextDate.empty())
//		{
//			return std::shared_ptr<StockDay>();
//		}
//		StockMarket day;
//		day.load(m_stock, nextDate, nextDate);
//		return day.stockDay(nextDate);
//	}
//	if (m_history.empty())
//	{
//		return std::shared_ptr<StockDay>();
//	}
//	auto itDay = m_history.find(date);
//	if (itDay == m_history.end())
//	{
//		m_history[date];
//	}
//	itDay = m_history.find(date);
//	if ((++itDay) == m_history.end())
//	{
//		m_history.erase(--itDay);
//		return std::shared_ptr<StockDay>();
//	}
//	std::shared_ptr<StockDay> result = itDay->second;
//	m_history.erase(--itDay);
//	return result;
//}
//
//BigNumber StockMarket::preOpen(const IntDateTime& date) const
//{
//	IntDateTime preDate = StockMysql::instance().getDatePre(m_stock, date);
//	if (preDate.empty())
//	{
//		return -1;
//	}
//	StockMysql::instance().
//	preDate
//	;
//	if (!getDatePre(date, preDate))
//	{
//		return -1;
//	}
//	return m_history.find(preDate)->second[OPEN];
//}
//
//BigNumber StockMarket::preHigh(const IntDateTime& date) const
//{
//	if (!dateExist(date))
//	{
//		return -1;
//	}
//	IntDateTime preDate;
//	if (!getDatePre(date, preDate))
//	{
//		return -1;
//	}
//	return m_history.find(preDate)->second[HIGH];
//}
//
//BigNumber StockMarket::preLow(const IntDateTime& date) const
//{
//	if (!dateExist(date))
//	{
//		return -1;
//	}
//	IntDateTime preDate;
//	if (!getDatePre(date, preDate))
//	{
//		return -1;
//	}
//	return m_history.find(preDate)->second[LOW];
//}
//
//BigNumber StockMarket::preClose(const IntDateTime& date) const
//{
//	if (!dateExist(date))
//	{
//		return -1;
//	}
//	IntDateTime preDate;
//	if (!getDatePre(date, preDate))
//	{
//		return -1;
//	}
//	return m_history.find(preDate)->second[CLOSE];
//}
//
//BigNumber StockMarket::nextOpen(const IntDateTime& date) const
//{
//	if (!dateExist(date))
//	{
//		return -1;
//	}
//	IntDateTime nextDate;
//	if (!getDateNext(date, nextDate))
//	{
//		return -1;
//	}
//	return m_history.find(nextDate)->second[OPEN];
//}
//
//BigNumber StockMarket::nextHigh(const IntDateTime& date) const
//{
//	if (!dateExist(date))
//	{
//		return -1;
//	}
//	IntDateTime nextDate;
//	if (!getDateNext(date, nextDate))
//	{
//		return -1;
//	}
//	return m_history.find(nextDate)->second[HIGH];
//}
//
//BigNumber StockMarket::nextLow(const IntDateTime& date) const
//{
//	if (!dateExist(date))
//	{
//		return -1;
//	}
//	IntDateTime nextDate;
//	if (!getDateNext(date, nextDate))
//	{
//		return -1;
//	}
//	return m_history.find(nextDate)->second[LOW];
//}
//
//BigNumber StockMarket::nextClose(const IntDateTime& date) const
//{
//	if (!dateExist(date))
//	{
//		return -1;
//	}
//	IntDateTime nextDate;
//	if (!getDateNext(date, nextDate))
//	{
//		return -1;
//	}
//	return m_history.find(nextDate)->second[CLOSE];
//}

bool StockMarket::dateExist(const IntDateTime& date) const
{
	return m_spStockMysql->dateExist(m_stock, date);
}

IntDateTime StockMarket::beginDate()
{
	return m_spStockMysql->beginDate(m_stock);
}

//bool StockMarket::getDatePre(const IntDateTime& date, IntDateTime& preDate) const
//{
//	if (!dateExist(date))
//	{
//		return false;
//	}
//	auto itDate = m_history.find(date);
//	if (itDate == m_history.begin())
//	{
//		return false;
//	}
//	preDate = (--itDate)->first;
//	return true;
//}

IntDateTime StockMarket::getDatePre(const IntDateTime& date) const
{
	return m_spStockMysql->getDatePre(m_stock, date);
}

//bool StockMarket::getDateNext(const IntDateTime& date, IntDateTime& nextDate) const
//{
//	if (!dateExist(date))
//	{
//		return false;
//	}
//	auto itDate = ++(m_history.find(date));
//	if (itDate == m_history.end())
//	{
//		return false;
//	}
//	nextDate = itDate->first;
//	return true;
//}

IntDateTime StockMarket::getDateNext(const IntDateTime& date) const
{
	return m_spStockMysql->getDateNext(m_stock, date);
}

//bool StockMarket::getMarketPre(const IntDateTime& date, IntDateTime& preDate, std::vector<BigNumber>& preData) const
//{
//	if (!dateExist(date))
//	{
//		return false;
//	}
//	if (!getDatePre(date, preDate))
//	{
//		return false;
//	}
//	preData = m_history.find(preDate)->second;
//	return true;
//}
//
//bool StockMarket::getMarketNext(const IntDateTime& date, IntDateTime& nextDate, std::vector<BigNumber>& nextData) const
//{
//	if (!dateExist(date))
//	{
//		return false;
//	}
//	if (!getDateNext(date, nextDate))
//	{
//		return false;
//	}
//	nextData = m_history.find(nextDate)->second;
//	return true;
//}

BigNumber StockMarket::getDays(const IntDateTime& date1, const IntDateTime& date2)
{
	if (date1 > date2)
	{
		return 0;
	}
	if (date1 == date2)
	{
		return 1;
	}
	return m_spStockMysql->getDays(m_stock, date1, date2);
}

//bool StockMarket::getMarketPre(const IntDateTime& date, int32_t days, std::map<IntDateTime, std::vector<BigNumber>>& preDaysData) const
//{
//	if (!dateExist(date))
//	{
//		return false;
//	}
//
//	IntDateTime preDate = date;
//	int32_t dataDays = days;
//	--days;
//	while (days-- != 0)
//	{
//		if (!getDatePre(preDate, preDate))
//		{
//			return false;
//		}
//	}
//	preDaysData.clear();
//	while (dataDays-- != 0)
//	{
//		preDaysData[preDate] = m_history.find(preDate)->second;
//		getDateNext(preDate, preDate);
//	}
//	return true;
//}
//
//bool StockMarket::getMarketNext(const IntDateTime& date, int32_t days, std::map<IntDateTime, std::vector<BigNumber>>& nextDaysData) const
//{
//	if (!dateExist(date))
//	{
//		return false;
//	}
//	IntDateTime nextDate = date;
//	int32_t dataDays = days;
//	while (days-- != 0)
//	{
//		if (!getDateNext(nextDate, nextDate))
//		{
//			return false;
//		}
//	}
//	nextDaysData.clear();
//	nextDate = date;
//	while (dataDays-- != 0)
//	{
//		getDateNext(nextDate, nextDate);
//		nextDaysData[nextDate] = m_history.find(nextDate)->second;
//	}
//	return true;
//}

int main()
{
	StockMarket market;
	market.load("603826", "2019-01-01", "2019-04-30");
	do 
	{
		auto ss = market.day();
		int x = 3;
	} while (market.next());
	return 0;
}