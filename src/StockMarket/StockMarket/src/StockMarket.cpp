#include "StockMarket.h"
#include "CStringManager/CStringManagerAPI.h"
#include "StockMysql/StockMysqlAPI.h"
#include "StockDay.h"
#include "StockData.h"

StockMarket::StockMarket():
m_isLoadPreDate(false),
m_stockData(nullptr)
{
	m_date.clear();
	m_stockData.reset(new StockData);
}

StockMarket::StockMarket(const StockMarket& stockMarket)
{
	m_stockData = stockMarket.m_stockData;
	m_date = stockMarket.m_date;
}

StockMarket StockMarket::operator=(const StockMarket& stockMarket)
{
	m_stockData = stockMarket.m_stockData;
	m_date = stockMarket.m_date;
	return *this;
}

void StockMarket::loadFromRedis(const std::string& stock, const IntDateTime& beginTime, const IntDateTime& endTime)
{
	m_stockData->m_stock = stock;
	IntDateTime preDate = StockMysql::instance().getDatePre(stock, beginTime);
	m_isLoadPreDate = !(preDate.empty());
	m_market = StockMysql::instance().readMarket(stock, m_isLoadPreDate ? preDate : beginTime, endTime);
}

void StockMarket::loadFromMysql(const std::string& stock)
{
	m_stockData->m_stock = stock;
	m_isLoadPreDate = false;
	m_market = StockMysql::instance().readMarketFromMysql(stock);
}

void StockMarket::load()
{
	if (m_market == nullptr || m_market->empty())
	{
		return;
	}

	clear();

	if (!m_isLoadPreDate)
	{
		std::shared_ptr<StockDay> spFirstDay(new StockDay);
		m_date = (*m_market)[0][0];
		spFirstDay->load(m_stockData->m_stock,
			(*m_market)[0][0],
			(*m_market)[0][1].c_str(),
			(*m_market)[0][2].c_str(),
			(*m_market)[0][3].c_str(),
			(*m_market)[0][4].c_str(),
			(BigNumber((*m_market)[0][1].c_str()) / 1.1).toPrec(2));
		m_stockData->m_history[(*m_market)[0][0]] = spFirstDay;
	}

	int32_t dayIndex = 0;
	while (dayIndex++ != m_market->size() - 1)
	{
		std::shared_ptr<StockDay> spDay(new StockDay);
		spDay->load(m_stockData->m_stock,
			(*m_market)[dayIndex][0],
			(*m_market)[dayIndex][1].c_str(),
			(*m_market)[dayIndex][2].c_str(),
			(*m_market)[dayIndex][3].c_str(),
			(*m_market)[dayIndex][4].c_str(),
			(*m_market)[dayIndex - 1][4].c_str());
		m_stockData->m_history[(*m_market)[dayIndex][0]] = spDay;
	}
	setFirstDate();
	m_market = nullptr;
}

std::string StockMarket::stock() const
{
	return m_stockData->m_stock;
}

std::string StockMarket::name() const
{
	return m_stockData->m_name;
}

bool StockMarket::empty() const
{
	return m_stockData->m_history.empty();
}

void StockMarket::clear()
{
	m_date.clear();
	m_stockData->m_history.clear();
}

std::shared_ptr<StockDay> StockMarket::day() const
{
	auto itDate = m_stockData->m_history.find(m_date);
	if (itDate == m_stockData->m_history.end())
	{
		return std::shared_ptr<StockDay>();
	}
	return itDate->second;
}

BigNumber StockMarket::fiveLine()
{
	return dayLine(5);
}

BigNumber StockMarket::tenLine()
{
	return dayLine(10);
}

BigNumber StockMarket::twentyLine()
{
	return dayLine(20);
}

BigNumber StockMarket::thirtyLine()
{
	return dayLine(30);
}

BigNumber StockMarket::yearLine()
{
	return dayLine(250);
}

BigNumber StockMarket::dayLine(int32_t days)
{
	if (days <= 0)
	{
		return 0;
	}
	BigNumber allClose = day()->close();
	if (days == 1)
	{
		return allClose;
	}
	IntDateTime date = m_date;
	int32_t index = 0;
	while (index++ != days - 1)
	{
		if (!previous())
		{
			setDate(date);
			return 0;
		}
		allClose = allClose + day()->close();
	}
	setDate(date);
	return (allClose.toPrec(6) / days).toPrec(2);
}

IntDateTime StockMarket::date() const
{
	return m_date;
}

bool StockMarket::setDate(const IntDateTime& date)
{
	if (m_stockData->m_history.find(date) == m_stockData->m_history.end())
	{
		return false;
	}
	m_date = date;
	return true;
}

bool StockMarket::setLastDate(const IntDateTime& date)
{
	if (m_stockData->m_history.empty() || date < m_stockData->m_history.begin()->first)
	{
		return false;
	}
	m_date = (--m_stockData->m_history.upper_bound(date))->first;
	return true;
}

int32_t StockMarket::getMemoryDays(const IntDateTime& date1, const IntDateTime& date2)
{
	if (date1 > date2)
	{
		return 0;
	}
	auto itDate1 = m_stockData->m_history.find(date1);
	auto itDate2 = m_stockData->m_history.find(date2);
	if (itDate1 == m_stockData->m_history.end() || itDate2 == m_stockData->m_history.end())
	{
		return -1;
	}
	if (date1 == date2)
	{
		return 1;
	}
	int32_t day = 1;
	while (true)
	{
		++day;
		++itDate1;
		if (itDate1 == itDate2)
		{
			return day;
		}
	}
}

IntDateTime StockMarket::getDateBefore(int32_t days)
{
	if (m_stockData->m_history.empty())
	{
		return IntDateTime(0, 0);
	}

	auto itCurrentDate = m_stockData->m_history.find(m_date);
	auto itFirstDate = m_stockData->m_history.begin();
	while (days-- != 0)
	{
		if (--itCurrentDate == itFirstDate)
		{
			return itFirstDate->first;
		}
	}
	return itCurrentDate->first;
}

int32_t StockMarket::days()
{
	return m_stockData->m_history.size();
}

void StockMarket::setFirstDate()
{
	if (m_stockData->m_history.empty())
	{
		return;
	}
	m_date = m_stockData->m_history.begin()->first;
}

void StockMarket::setEndDate()
{
	if (m_stockData->m_history.empty())
	{
		return;
	}
	m_date = m_stockData->m_history.rbegin()->first;
}

bool StockMarket::previous()
{
	if (m_stockData->m_history.empty())
	{
		return false;
	}
	auto itDate = m_stockData->m_history.find(m_date);
	if (itDate == m_stockData->m_history.end())
	{
		return false;
	}
	if (itDate == m_stockData->m_history.begin())
	{
		return false;
	}
	m_date = (--itDate)->first;
	return true;
}

bool StockMarket::next()
{
	if (m_stockData->m_history.empty())
	{
		return false;
	}
	auto itDate = m_stockData->m_history.find(m_date);
	if (itDate == m_stockData->m_history.end())
	{
		return false;
	}
	if ((++itDate) == m_stockData->m_history.end())
	{
		return false;
	}
	m_date = itDate->first;
	return true;
}

void StockMarket::setStock(const std::string& stock)
{
	m_stockData->m_stock = stock;
}

std::shared_ptr<StockDay> StockMarket::stockDay(const IntDateTime& date) const
{
	StockMarket day;
	day.loadFromRedis(m_stockData->m_stock, date, date);
	day.load();
	return day.day();
}

//std::shared_ptr<StockDay> StockMarket::stockPreDay(const IntDateTime& date)
//{
//	IntDateTime preDate = StockMysql::instance().getDatePre(m_stockData->m_stock, date);
//	if (preDate.empty())
//	{
//		return std::shared_ptr<StockDay>();
//	}
//	StockMarket day;
//	day.load(m_stockData->m_stock, preDate, preDate);
//	return day.stockDay(preDate);
//}
//
//std::shared_ptr<StockDay> StockMarket::stockNextDay(const IntDateTime& date, bool useMysql)
//{
//	if (useMysql)
//	{
//		IntDateTime nextDate = StockMysql::instance().getDateNext(m_stockData->m_stock, date);
//		if (nextDate.empty())
//		{
//			return std::shared_ptr<StockDay>();
//		}
//		StockMarket day;
//		day.load(m_stockData->m_stock, nextDate, nextDate);
//		return day.stockDay(nextDate);
//	}
//	if (m_stockData->m_history.empty())
//	{
//		return std::shared_ptr<StockDay>();
//	}
//	auto itDay = m_stockData->m_history.find(date);
//	if (itDay == m_stockData->m_history.end())
//	{
//		m_stockData->m_history[date];
//	}
//	itDay = m_stockData->m_history.find(date);
//	if ((++itDay) == m_stockData->m_history.end())
//	{
//		m_stockData->m_history.erase(--itDay);
//		return std::shared_ptr<StockDay>();
//	}
//	std::shared_ptr<StockDay> result = itDay->second;
//	m_stockData->m_history.erase(--itDay);
//	return result;
//}
//
//BigNumber StockMarket::preOpen(const IntDateTime& date) const
//{
//	IntDateTime preDate = StockMysql::instance().getDatePre(m_stockData->m_stock, date);
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
//	return m_stockData->m_history.find(preDate)->second[OPEN];
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
//	return m_stockData->m_history.find(preDate)->second[HIGH];
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
//	return m_stockData->m_history.find(preDate)->second[LOW];
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
//	return m_stockData->m_history.find(preDate)->second[CLOSE];
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
//	return m_stockData->m_history.find(nextDate)->second[OPEN];
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
//	return m_stockData->m_history.find(nextDate)->second[HIGH];
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
//	return m_stockData->m_history.find(nextDate)->second[LOW];
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
//	return m_stockData->m_history.find(nextDate)->second[CLOSE];
//}

bool StockMarket::dateExist(const IntDateTime& date) const
{
	return StockMysql::instance().dateExist(m_stockData->m_stock, date);
}

IntDateTime StockMarket::beginDate()
{
	return StockMysql::instance().beginDate(m_stockData->m_stock);
}

//bool StockMarket::getDatePre(const IntDateTime& date, IntDateTime& preDate) const
//{
//	if (!dateExist(date))
//	{
//		return false;
//	}
//	auto itDate = m_stockData->m_history.find(date);
//	if (itDate == m_stockData->m_history.begin())
//	{
//		return false;
//	}
//	preDate = (--itDate)->first;
//	return true;
//}

IntDateTime StockMarket::getDatePre(const IntDateTime& date) const
{
	return StockMysql::instance().getDatePre(m_stockData->m_stock, date);
}

//bool StockMarket::getDateNext(const IntDateTime& date, IntDateTime& nextDate) const
//{
//	if (!dateExist(date))
//	{
//		return false;
//	}
//	auto itDate = ++(m_stockData->m_history.find(date));
//	if (itDate == m_stockData->m_history.end())
//	{
//		return false;
//	}
//	nextDate = itDate->first;
//	return true;
//}

IntDateTime StockMarket::getDateNext(const IntDateTime& date) const
{
	return StockMysql::instance().getDateNext(m_stockData->m_stock, date);
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
//	preData = m_stockData->m_history.find(preDate)->second;
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
//	nextData = m_stockData->m_history.find(nextDate)->second;
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
	return StockMysql::instance().getDays(m_stockData->m_stock, date1, date2);
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
//		preDaysData[preDate] = m_stockData->m_history.find(preDate)->second;
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
//		nextDaysData[nextDate] = m_stockData->m_history.find(nextDate)->second;
//	}
//	return true;
//}

//int main()
//{
//	auto allStock = StockMysql::instance().allStock();
//	int32_t begin = ::GetTickCount();
//
//	CStopWatch stopWatch4;
//	
//	int32_t index = -1;
//	while (index++ != allStock.size() - 1)
//	{
//		StockMarket market;
//		stopWatch4.Run();
//		//market.loadFromDb("603826","2017-04-15","2019-05-30");
//		market.loadFromDb(allStock[index]);
//		market.load();
//		stopWatch4.Stop();
//		RCSend("%d", index + 1);
//	}
//	RCSend("time = %.2lf",(::GetTickCount() - begin) / 60000.0);
//	
//	StockMarket market;
//	market.printStopWatch();
//	RCSend("4 = %d", stopWatch4.GetWatchTime());
//	//do 
//	//{
//	//	auto ss = market.day();
//	//	int x = 3;
//	//} while (market.next());
//	return 0;
//}