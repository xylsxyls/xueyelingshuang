#pragma once
#include <string>
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

	friend IntDateTime operator+(const IntDateTime& intDateTime, int seconds);
	friend IntDateTime operator+(int seconds, const IntDateTime& intDateTime);
	friend IntDateTime operator-(const IntDateTime& intDateTime, int seconds);
	friend int operator-(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2);

	friend bool operator< (const IntDateTime& intDateTime1, const IntDateTime& intDateTime2);
	friend bool operator> (const IntDateTime& intDateTime1, const IntDateTime& intDateTime2);
	friend bool operator<=(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2);
	friend bool operator>=(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2);
	friend bool operator==(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2);
	friend bool operator!=(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2);
	friend bool operator==(const IntDateTime& intDateTime, int num);
	friend bool operator==(int num, const IntDateTime& intDateTime);

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

	string toString();
	string dateToString();
	string dateNumToString();
	string timeToString();
};