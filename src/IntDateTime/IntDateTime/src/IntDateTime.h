#pragma once
#include <string>
#include <time.h>
#include <stdint.h>
#include "IntDateTimeMacro.h"

class IntDateTimeAPI IntDateTime
{
public:
	int32_t m_date;
	int32_t m_time;

	//默认获取本地当前时间
	IntDateTime();
	IntDateTime(const IntDateTime& intDateTime);
	IntDateTime(int32_t date, int32_t time);
	IntDateTime(const std::string& time);
	IntDateTime(time_t time);

	IntDateTime operator=(const IntDateTime& intDateTime);
	IntDateTime operator=(const std::string& intDateTime);
	IntDateTime operator=(const time_t& time);

    friend IntDateTime operator+(const IntDateTime& intDateTime, int32_t seconds)
	{
        struct tm time;
        time.tm_year = intDateTime.getYear() - 1900;
        time.tm_mon = intDateTime.getMonth() - 1;
        time.tm_mday = intDateTime.getDay();
        time.tm_hour = intDateTime.getHour();
        time.tm_min = intDateTime.getMin();
        time.tm_sec = intDateTime.getSeconds();
        time_t timeint64 = mktime(&time);
        IntDateTime result;
        result = timeint64 + seconds;
        return result;
    }
    friend IntDateTime operator+(int32_t seconds, const IntDateTime& intDateTime)
	{
		struct tm time;
		time.tm_year = intDateTime.getYear() - 1900;
		time.tm_mon = intDateTime.getMonth() - 1;
		time.tm_mday = intDateTime.getDay();
		time.tm_hour = intDateTime.getHour();
		time.tm_min = intDateTime.getMin();
		time.tm_sec = intDateTime.getSeconds();
        time_t timeint64 = mktime(&time);
        IntDateTime result;
		result = timeint64 + seconds;
        return result;
    }
    friend IntDateTime operator-(const IntDateTime& intDateTime, int32_t seconds)
	{
		struct tm time;
		time.tm_year = intDateTime.getYear() - 1900;
		time.tm_mon = intDateTime.getMonth() - 1;
		time.tm_mday = intDateTime.getDay();
		time.tm_hour = intDateTime.getHour();
		time.tm_min = intDateTime.getMin();
		time.tm_sec = intDateTime.getSeconds();
        time_t timeint64 = mktime(&time);
        IntDateTime result;
		result = timeint64 - seconds;
        return result;
    }
    friend int32_t operator-(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2)
	{
		struct tm time;
		time.tm_year = intDateTime1.getYear() - 1900;
		time.tm_mon = intDateTime1.getMonth() - 1;
		time.tm_mday = intDateTime1.getDay();
		time.tm_hour = intDateTime1.getHour();
		time.tm_min = intDateTime1.getMin();
		time.tm_sec = intDateTime1.getSeconds();
        time_t timeint64 = mktime(&time);

        struct tm timeSub;
		timeSub.tm_year = intDateTime2.getYear() - 1900;
		timeSub.tm_mon = intDateTime2.getMonth() - 1;
		timeSub.tm_mday = intDateTime2.getDay();
		timeSub.tm_hour = intDateTime2.getHour();
		timeSub.tm_min = intDateTime2.getMin();
		timeSub.tm_sec = intDateTime2.getSeconds();
        time_t timeint64Sub = mktime(&timeSub);

		return (int32_t)difftime(timeint64, timeint64Sub);
    }

    friend bool operator< (const IntDateTime& intDateTime1, const IntDateTime& intDateTime2)
	{
		if (intDateTime1.m_date < intDateTime2.m_date)
		{
			return true;
		}
		else if (intDateTime1.m_date > intDateTime2.m_date)
		{
			return false;
		}
        else
		{
			if (intDateTime1.m_time < intDateTime2.m_time)
			{
				return true;
			}
			else if (intDateTime1.m_time > intDateTime2.m_time)
			{
				return false;
			}
			else
			{
				return false;
			}
        }
    }

    friend bool operator> (const IntDateTime& intDateTime1, const IntDateTime& intDateTime2)
	{
		if (intDateTime1.m_date < intDateTime2.m_date)
		{
			return false;
		}
		else if (intDateTime1.m_date > intDateTime2.m_date)
		{
			return true;
		}
        else
		{
			if (intDateTime1.m_time < intDateTime2.m_time)
			{
				return false;
			}
			else if (intDateTime1.m_time > intDateTime2.m_time)
			{
				return true;
			}
			else
			{
				return false;
			}
        }
    }

    friend bool operator<=(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2)
	{
		if (intDateTime1.m_date < intDateTime2.m_date)
		{
			return true;
		}
		else if (intDateTime1.m_date > intDateTime2.m_date)
		{
			return false;
		}
        else
		{
			if (intDateTime1.m_time < intDateTime2.m_time)
			{
				return true;
			}
			else if (intDateTime1.m_time > intDateTime2.m_time)
			{
				return false;
			}
			else
			{
				return true;
			}
        }
    }

    friend bool operator>=(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2)
	{
		if (intDateTime1.m_date < intDateTime2.m_date)
		{
			return false;
		}
		else if (intDateTime1.m_date > intDateTime2.m_date)
		{
			return true;
		}
        else
		{
			if (intDateTime1.m_time < intDateTime2.m_time)
			{
				return false;
			}
			else if (intDateTime1.m_time > intDateTime2.m_time)
			{
				return true;
			}
			else
			{
				return true;
			}
        }
    }

    friend bool operator==(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2)
	{
		if (intDateTime1.m_date == intDateTime2.m_date && intDateTime1.m_time == intDateTime2.m_time)
		{
			return true;
		}
		else
		{
			return false;
		}
    }

    friend bool operator!=(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2)
	{
		if (intDateTime1.m_date == intDateTime2.m_date && intDateTime1.m_time == intDateTime2.m_time)
		{
			return false;
		}
		else
		{
			return true;
		}
    }

    friend bool operator==(const IntDateTime& intDateTime, int32_t num)
	{
		if (num != 0)
		{
			return false;
		}
		if (intDateTime.getDate() == 0 && intDateTime.getTime() == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
    }

    friend bool operator==(int32_t num, const IntDateTime& intDateTime)
	{
		if (num != 0)
		{
			return false;
		}
		if (intDateTime.getDate() == 0 && intDateTime.getTime() == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
    }

	void setTime(int32_t date, int32_t time);
	void setTime(const std::string& time);
	void setTime(time_t time);

	int32_t getDate() const;
	int32_t getTime() const;

	int32_t getYear() const;
	int32_t getMonth() const;
	int32_t getDay() const;
	int32_t getHour() const;
	int32_t getMin() const;
	int32_t getSeconds() const;

	std::string toString() const;
	std::string dateToString() const;
	std::string dateNumToString() const;
	std::string timeToString() const;
};