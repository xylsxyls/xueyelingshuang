#pragma once
#include <string>
#include <time.h>
#include "IntDateTimeMacro.h"
using namespace std;

class IntDateTimeAPI IntDateTime{
public:
	int date_;
	int time_;

	//默认获取本地当前时间
	IntDateTime();
	IntDateTime(const IntDateTime& intDateTime);
	IntDateTime(int date, int time);
	IntDateTime(const string& time);
	IntDateTime(time_t time);

	IntDateTime operator=(const IntDateTime& intDateTime);
	IntDateTime operator=(const string& intDateTime);
	IntDateTime operator=(const time_t& time);

    friend IntDateTime operator+(const IntDateTime& intDateTime, int seconds){
        struct tm Time;
        Time.tm_year = intDateTime.getYear() - 1900;
        Time.tm_mon = intDateTime.getMonth() - 1;
        Time.tm_mday = intDateTime.getDay();
        Time.tm_hour = intDateTime.getHour();
        Time.tm_min = intDateTime.getMin();
        Time.tm_sec = intDateTime.getSeconds();
        time_t Timeint64 = mktime(&Time);
        IntDateTime result;
        result = Timeint64 + seconds;
        return result;
    }
    friend IntDateTime operator+(int seconds, const IntDateTime& intDateTime){
        struct tm Time;
        Time.tm_year = intDateTime.getYear() - 1900;
        Time.tm_mon = intDateTime.getMonth() - 1;
        Time.tm_mday = intDateTime.getDay();
        Time.tm_hour = intDateTime.getHour();
        Time.tm_min = intDateTime.getMin();
        Time.tm_sec = intDateTime.getSeconds();
        time_t Timeint64 = mktime(&Time);
        IntDateTime result;
        result = Timeint64 + seconds;
        return result;
    }
    friend IntDateTime operator-(const IntDateTime& intDateTime, int seconds){
        struct tm Time;
        Time.tm_year = intDateTime.getYear() - 1900;
        Time.tm_mon = intDateTime.getMonth() - 1;
        Time.tm_mday = intDateTime.getDay();
        Time.tm_hour = intDateTime.getHour();
        Time.tm_min = intDateTime.getMin();
        Time.tm_sec = intDateTime.getSeconds();
        time_t Timeint64 = mktime(&Time);
        IntDateTime result;
        result = Timeint64 - seconds;
        return result;
    }
    friend int operator-(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2){
        struct tm Time;
        Time.tm_year = intDateTime1.getYear() - 1900;
        Time.tm_mon = intDateTime1.getMonth() - 1;
        Time.tm_mday = intDateTime1.getDay();
        Time.tm_hour = intDateTime1.getHour();
        Time.tm_min = intDateTime1.getMin();
        Time.tm_sec = intDateTime1.getSeconds();
        time_t Timeint64 = mktime(&Time);

        struct tm TimeSub;
        TimeSub.tm_year = intDateTime2.getYear() - 1900;
        TimeSub.tm_mon = intDateTime2.getMonth() - 1;
        TimeSub.tm_mday = intDateTime2.getDay();
        TimeSub.tm_hour = intDateTime2.getHour();
        TimeSub.tm_min = intDateTime2.getMin();
        TimeSub.tm_sec = intDateTime2.getSeconds();
        time_t Timeint64Sub = mktime(&TimeSub);

        return (int)difftime(Timeint64, Timeint64Sub);
    }

    friend bool operator< (const IntDateTime& intDateTime1, const IntDateTime& intDateTime2){
        if (intDateTime1.date_ < intDateTime2.date_) return true;
        else if (intDateTime1.date_ > intDateTime2.date_) return false;
        else{
            if (intDateTime1.time_ < intDateTime2.time_) return true;
            else if (intDateTime1.time_ > intDateTime2.time_) return false;
            else return false;
        }
    }

    friend bool operator> (const IntDateTime& intDateTime1, const IntDateTime& intDateTime2){
        if (intDateTime1.date_ < intDateTime2.date_) return false;
        else if (intDateTime1.date_ > intDateTime2.date_) return true;
        else{
            if (intDateTime1.time_ < intDateTime2.time_) return false;
            else if (intDateTime1.time_ > intDateTime2.time_) return true;
            else return false;
        }
    }

    friend bool operator<=(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2){
        if (intDateTime1.date_ < intDateTime2.date_) return true;
        else if (intDateTime1.date_ > intDateTime2.date_) return false;
        else{
            if (intDateTime1.time_ < intDateTime2.time_) return true;
            else if (intDateTime1.time_ > intDateTime2.time_) return false;
            else return true;
        }
    }

    friend bool operator>=(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2){
        if (intDateTime1.date_ < intDateTime2.date_) return false;
        else if (intDateTime1.date_ > intDateTime2.date_) return true;
        else{
            if (intDateTime1.time_ < intDateTime2.time_) return false;
            else if (intDateTime1.time_ > intDateTime2.time_) return true;
            else return true;
        }
    }

    friend bool operator==(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2){
        if (intDateTime1.date_ == intDateTime2.date_ && intDateTime1.time_ == intDateTime2.time_) return true;
        else return false;
    }

    friend bool operator!=(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2){
        if (intDateTime1.date_ == intDateTime2.date_ && intDateTime1.time_ == intDateTime2.time_) return false;
        else return true;
    }

    friend bool operator==(const IntDateTime& intDateTime, int num){
        if (num != 0) return false;
        if (intDateTime.getDate() == 0 && intDateTime.getTime() == 0) return true;
        else return false;
    }

    friend bool operator==(int num, const IntDateTime& intDateTime){
        if (num != 0) return false;
        if (intDateTime.getDate() == 0 && intDateTime.getTime() == 0) return true;
        else return false;
    }

	void setTime(int date, int time);
	void setTime(const string& time);
	void setTime(time_t time);

	int getDate()const;
	int getTime()const;

	int getYear()const;
	int getMonth()const;
	int getDay()const;
	int getHour()const;
	int getMin()const;
	int getSeconds()const;

	string toString()const;
	string dateToString()const;
	string dateNumToString()const;
	string timeToString()const;
};