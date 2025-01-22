#include "IntDateTime.h"
#include "CStringManager/CStringManagerAPI.h"

IntDateTime::IntDateTime()
{
	time_t timep;
	struct tm *p;
	time(&timep);
	//?取得当地时间
	p = localtime(&timep);
	int32_t year = 1900 + p->tm_year;
	int32_t month = 1 + p->tm_mon;
	int32_t day = p->tm_mday;
	int32_t hour = p->tm_hour;
	int32_t min = p->tm_min;
	int32_t seconds = p->tm_sec;
	std::string strLocalDate;
	CStringManager::Format(strLocalDate, "%d%02d%02d", year, month, day);
	m_date = atoi(strLocalDate.c_str());
	std::string strLocalTime;
	CStringManager::Format(strLocalTime, "%02d%02d%02d000", hour, min, seconds);
	m_time = atoi(strLocalTime.c_str());
}

IntDateTime::IntDateTime(const IntDateTime& intDateTime)
{
	m_date = intDateTime.m_date;
	m_time = intDateTime.m_time;
}

IntDateTime::IntDateTime(int32_t date, int32_t time)
{
	m_date = date;
	m_time = time;
}

IntDateTime::IntDateTime(const std::string& time)
{
	std::vector<std::string> vecDateTime = CStringManager::split(time, " ");
	if (vecDateTime.size() != 2 && vecDateTime.size() != 1)
	{
		printf("解析时间出错");
		return;
	}
	std::string strDate = vecDateTime.at(0);
	std::string strTime = (vecDateTime.size() == 1) ? "00:00:00" : vecDateTime.at(1);
	CStringManager::Replace(strDate, "-", "");
	m_date = atoi(strDate.c_str());
	CStringManager::Replace(strTime, ":", "");
	m_time = atoi(strTime.c_str()) * 1000;
}

IntDateTime::IntDateTime(time_t time)
{
	struct tm *p;
	//?取得当地时间
	p = localtime(&time);
	int32_t year = 1900 + p->tm_year;
	int32_t month = 1 + p->tm_mon;
	int32_t day = p->tm_mday;
	int32_t hour = p->tm_hour;
	int32_t min = p->tm_min;
	int32_t seconds = p->tm_sec;
	std::string strLocalDate;
	CStringManager::Format(strLocalDate, "%d%02d%02d", year, month, day);
	m_date = atoi(strLocalDate.c_str());
	std::string strLocalTime;
	CStringManager::Format(strLocalTime, "%02d%02d%02d000", hour, min, seconds);
	m_time = atoi(strLocalTime.c_str());
}

IntDateTime::IntDateTime(const char* time)
{
	std::vector<std::string> vecDateTime = CStringManager::split(time, " ");
	if (vecDateTime.size() != 2 && vecDateTime.size() != 1)
	{
		printf("解析时间出错");
		return;
	}
	std::string strDate = vecDateTime.at(0);
	std::string strTime = (vecDateTime.size() == 1) ? "00:00:00" : vecDateTime.at(1);
	CStringManager::Replace(strDate, "-", "");
	m_date = atoi(strDate.c_str());
	CStringManager::Replace(strTime, ":", "");
	m_time = atoi(strTime.c_str()) * 1000;
}

IntDateTime IntDateTime::operator = (const IntDateTime& intDateTime)
{
	m_date = intDateTime.m_date;
	m_time = intDateTime.m_time;
	return *this;
}

IntDateTime IntDateTime::operator=(const std::string& intDateTime)
{
	std::vector<std::string> vecDateTime = CStringManager::split(intDateTime, " ");
	if (vecDateTime.size() != 2 && vecDateTime.size() != 1)
	{
		printf("解析时间出错");
		return *this;
	}
	std::string strDate = vecDateTime.at(0);
	std::string strTime = (vecDateTime.size() == 1) ? "00:00:00" : vecDateTime.at(1);
	CStringManager::Replace(strDate, "-", "");
	m_date = atoi(strDate.c_str());
	CStringManager::Replace(strTime, ":", "");
	m_time = atoi(strTime.c_str()) * 1000;
	return *this;
}

IntDateTime IntDateTime::operator=(const time_t& time)
{
	struct tm *p;
	//?取得当地时间
	p = localtime(&time);
	int32_t year = 1900 + p->tm_year;
	int32_t month = 1 + p->tm_mon;
	int32_t day = p->tm_mday;
	int32_t hour = p->tm_hour;
	int32_t min = p->tm_min;
	int32_t seconds = p->tm_sec;
	std::string strLocalDate;
	CStringManager::Format(strLocalDate, "%d%02d%02d", year, month, day);
	m_date = atoi(strLocalDate.c_str());
	std::string strLocalTime;
	CStringManager::Format(strLocalTime, "%02d%02d%02d000", hour, min, seconds);
	m_time = atoi(strLocalTime.c_str());
	return *this;
}

bool IntDateTime::empty() const
{
	return m_date == 0 && m_time == 0;
}

void IntDateTime::clear()
{
	m_date = 0;
	m_time = 0;
}

void IntDateTime::setTime(int32_t date, int32_t time)
{
	m_date = date;
	m_time = time;
}

void IntDateTime::setTime(const std::string& time)
{
	std::vector<std::string> vecDateTime = CStringManager::split(time, " ");
	if (vecDateTime.size() != 2 && vecDateTime.size() != 1)
	{
		printf("解析时间出错");
		return;
	}
	std::string strDate = vecDateTime.at(0);
	std::string strTime = (vecDateTime.size() == 1) ? "00:00:00" : vecDateTime.at(1);
	CStringManager::Replace(strDate, "-", "");
	m_date = atoi(strDate.c_str());
	CStringManager::Replace(strTime, ":", "");
	m_time = atoi(strTime.c_str()) * 1000;
}

void IntDateTime::setTime(time_t time)
{
	struct tm *p;
	//?取得当地时间
	p = localtime(&time);
	int32_t year = 1900 + p->tm_year;
	int32_t month = 1 + p->tm_mon;
	int32_t day = p->tm_mday;
	int32_t hour = p->tm_hour;
	int32_t min = p->tm_min;
	int32_t seconds = p->tm_sec;
	std::string strLocalDate;
	CStringManager::Format(strLocalDate, "%d%02d%02d", year, month, day);
	m_date = atoi(strLocalDate.c_str());
	std::string strLocalTime;
	CStringManager::Format(strLocalTime, "%02d%02d%02d000", hour, min, seconds);
	m_time = atoi(strLocalTime.c_str());
}

int32_t IntDateTime::getDate() const
{
	return m_date;
}

int32_t IntDateTime::getTime() const
{
	return m_time;
}

int32_t IntDateTime::getYear() const
{
	return m_date / 10000;
}

int32_t IntDateTime::getMonth() const
{
	return (m_date % 10000) / 100;
}

int32_t IntDateTime::getDay() const
{
	return (m_date % 10000) % 100;
}

int32_t IntDateTime::getHour() const
{
	return m_time / 10000000;
}

int32_t IntDateTime::getMin() const
{
	return (m_time % 10000000) / 100000;
}

int32_t IntDateTime::getSeconds() const
{
	return (m_time % 100000) / 1000;
}

std::string IntDateTime::toString() const
{
	std::string result;
	CStringManager::Format(result, "%d-%02d-%02d %02d:%02d:%02d", m_date / 10000, (m_date % 10000) / 100, (m_date % 10000) % 100, m_time / 10000000, (m_time % 10000000) / 100000, (m_time % 100000) / 1000);
	return result;
}

std::string IntDateTime::dateToString() const
{
	std::string result;
	CStringManager::Format(result, "%d-%02d-%02d", m_date / 10000, (m_date % 10000) / 100, (m_date % 10000) % 100);
	return result;
}

std::string IntDateTime::dateNumToString() const
{
	std::string result;
	CStringManager::Format(result, "%d%02d%02d", m_date / 10000, (m_date % 10000) / 100, (m_date % 10000) % 100);
	return result;
}

std::string IntDateTime::timeToString() const
{
	std::string result;
	CStringManager::Format(result, "%02d:%02d:%02d", m_time / 10000000, (m_time % 10000000) / 100000, (m_time % 100000) / 1000);
	return result;
}