#include "IntDateTime.h"
#include <cerrno>
#include <limits>
#include <cstdlib>
#include "CStringManager/CStringManagerAPI.h"

IntDateTime::IntDateTime()
{
    setTime(::time(nullptr));
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
    clear();
    setTime(time);
}

IntDateTime::IntDateTime(time_t time)
{
    setTime(time);
}

IntDateTime::IntDateTime(const char* time)
{
    clear();
    if (time != nullptr)
    {
        setTime(std::string(time));
    }
}

IntDateTime IntDateTime::operator = (const IntDateTime& intDateTime)
{
	m_date = intDateTime.m_date;
	m_time = intDateTime.m_time;
	return *this;
}

IntDateTime IntDateTime::operator=(const std::string& intDateTime)
{
    setTime(intDateTime);
    return *this;
}

IntDateTime IntDateTime::operator=(const time_t& time)
{
    setTime(time);
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
		return;
	}
	std::string strDate = vecDateTime.at(0);
	std::string strTime = (vecDateTime.size() == 1) ? "00:00:00" : vecDateTime.at(1);
	const bool negativeYear = !strDate.empty() && strDate[0] == '-';
    CStringManager::Replace(strDate, "-", "");
    if (negativeYear) { strDate.insert(0, 1, '-'); }
	CStringManager::Replace(strTime, ":", "");
    // 保留十进制前缀的解析规则；检查转换和毫秒缩放后再同时发布两个字段。
    errno = 0;
    const long date = std::strtol(strDate.c_str(), nullptr, 10);
    if (errno == ERANGE || date < INT32_MIN || date > INT32_MAX)
    {
        return;
    }
    const long clock = std::strtol(strTime.c_str(), nullptr, 10);
    if (errno == ERANGE || clock < INT32_MIN / 1000 || clock > INT32_MAX / 1000)
    {
        return;
    }
    m_date = static_cast<int32_t>(date);
    m_time = static_cast<int32_t>(clock) * 1000;
}

void IntDateTime::setTime(time_t time)
{
    struct tm timeInfo;
    if (!localTime(time, timeInfo))
    {
        clear();
        return;
    }
    const int64_t year = static_cast<int64_t>(timeInfo.tm_year) + 1900;
    const int32_t monthDay = (timeInfo.tm_mon + 1) * 100 + timeInfo.tm_mday;
    const int64_t date = year * 10000 + (year < 0 ? -monthDay : monthDay);
    if (date < INT32_MIN || date > INT32_MAX)
    {
        clear();
        return;
    }
    m_date = static_cast<int32_t>(date);
    m_time = ((timeInfo.tm_hour * 100 + timeInfo.tm_min) * 100 + timeInfo.tm_sec) * 1000;
}

bool IntDateTime::toTimestamp(time_t& timestamp) const
{
    if (empty())
    {
        return false;
    }
    struct tm parts = {};
    parts.tm_year = getYear() - 1900;
    parts.tm_mon = getMonth() - 1;
    parts.tm_mday = getDay();
    parts.tm_hour = getHour();
    parts.tm_min = getMin();
    parts.tm_sec = getSeconds();
    parts.tm_isdst = -1;
    errno = 0;
    timestamp = mktime(&parts);
#ifdef _WIN32
    return timestamp != static_cast<time_t>(-1);
#else
    return timestamp != static_cast<time_t>(-1) || errno == 0;
#endif
}

IntDateTime IntDateTime::shiftSeconds(int64_t seconds) const
{
    time_t base;
    if (!toTimestamp(base) ||
        (seconds > 0 && base > (std::numeric_limits<time_t>::max)() - seconds) ||
        (seconds < 0 && base < (std::numeric_limits<time_t>::min)() - seconds))
    {
        return IntDateTime(0, 0);
    }
    return IntDateTime(static_cast<time_t>(base + seconds));
}

bool IntDateTime::localTime(time_t time, struct tm& timeInfo)
{
	timeInfo = tm();
#ifdef _WIN32
	return localtime_s(&timeInfo, &time) == 0;
#elif defined(__unix__) || defined(__APPLE__)
	return localtime_r(&time, &timeInfo) != nullptr;
#else
	(void)time;
	return false;
#endif
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
	return std::abs((m_date % 10000) / 100);
}

int32_t IntDateTime::getDay() const
{
	return std::abs(m_date % 100);
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
	CStringManager::Format(result, "%d-%02d-%02d %02d:%02d:%02d", getYear(), getMonth(), getDay(), m_time / 10000000, (m_time % 10000000) / 100000, (m_time % 100000) / 1000);
	return result;
}

std::string IntDateTime::dateToString() const
{
	std::string result;
	CStringManager::Format(result, "%d-%02d-%02d", getYear(), getMonth(), getDay());
	return result;
}

std::string IntDateTime::dateNumToString() const
{
	std::string result;
	CStringManager::Format(result, "%d%02d%02d", getYear(), getMonth(), getDay());
	return result;
}

std::string IntDateTime::timeToString() const
{
	std::string result;
	CStringManager::Format(result, "%02d:%02d:%02d", m_time / 10000000, (m_time % 10000000) / 100000, (m_time % 100000) / 1000);
	return result;
}

// 使用示例开始。单独编译本文件时可启用下方的 main。
//#include <ctime>
//#include <iostream>
//
//int main()
//{
//    // 输入和展示使用本地时间。以预约开始时间为基准，计算签到和结束窗口。
//    IntDateTime start("2026-08-09 09:30:00");
//    if (start.empty())
//    {
//        return 1;
//    }
//    const IntDateTime checkIn = start - 600;
//    const IntDateTime finish = start + 3600;
//    std::cout << start.dateToString() << " check-in " << checkIn.timeToString()
//        << ", meeting " << start.timeToString() << " to " << finish.timeToString() << '\n';
//    std::cout << "duration=" << finish - start << " seconds, archive=" << start.dateNumToString() << '\n';
//
//    IntDateTime stored(start.getDate(), start.getTime());
//    IntDateTime now(std::time(nullptr));
//    std::cout << "stored=" << stored.toString() << ", year=" << stored.getYear()
//        << ", month=" << stored.getMonth() << ", day=" << stored.getDay() << '\n';
//    if (now >= checkIn && now < finish)
//    {
//        std::cout << "Registration window is open\n";
//    }
//    // 改期可直接设置日期与时刻整数；clear 表示取消预约。
//    stored.setTime(20260810, 143000000);
//    std::cout << "rescheduled=" << stored.getHour() << ':' << stored.getMin() << ':' << stored.getSeconds() << '\n';
//    stored.clear();
//    return stored.empty() ? 0 : 1;
//}
// 使用示例结束。