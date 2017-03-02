#include "IntDateTime.h"
#include <time.h>
#include "CStringManager/CStringManagerAPI.h"

IntDateTime::IntDateTime(){
	time_t timep;
	struct tm *p;
	time(&timep);
	//?取得当地时间
	p = localtime(&timep);
	int year = 1900 + p->tm_year;
	int month = 1 + p->tm_mon;
	int day = p->tm_mday;
	int hour = p->tm_hour;
	int min = p->tm_min;
	int seconds = p->tm_sec;
	string strLocalDate;
	CStringManager::Format(strLocalDate, "%d%02d%02d", year, month, day);
	date_ = atoi(strLocalDate.c_str());
	string strLocalTime;
	CStringManager::Format(strLocalTime, "%02d%02d%02d000", hour, min, seconds);
	time_ = atoi(strLocalTime.c_str());
}

IntDateTime::IntDateTime(const IntDateTime& intDateTime){
	this->date_ = intDateTime.date_;
	this->time_ = intDateTime.time_;
}

IntDateTime::IntDateTime(int date, int time){
	this->date_ = date;
	this->time_ = time;
}

IntDateTime::IntDateTime(const string& time){
	vector<string> vecDateTime = CStringManager::split(time, " ");
	if (vecDateTime.size() != 2){
		printf("解析时间出错");
		return;
	}
	string strDate = vecDateTime.at(0);
	string strTime = vecDateTime.at(1);
	CStringManager::Replace(strDate, "-", "");
	date_ = atoi(strDate.c_str());
	CStringManager::Replace(strTime, ":", "");
	time_ = atoi(strTime.c_str()) * 1000;
}

IntDateTime::IntDateTime(time_t time){
	struct tm *p;
	//?取得当地时间
	p = localtime(&time);
	int year = 1900 + p->tm_year;
	int month = 1 + p->tm_mon;
	int day = p->tm_mday;
	int hour = p->tm_hour;
	int min = p->tm_min;
	int seconds = p->tm_sec;
	string strLocalDate;
	CStringManager::Format(strLocalDate, "%d%02d%02d", year, month, day);
	date_ = atoi(strLocalDate.c_str());
	string strLocalTime;
	CStringManager::Format(strLocalTime, "%02d%02d%02d000", hour, min, seconds);
	time_ = atoi(strLocalTime.c_str());
}

IntDateTime IntDateTime::operator = (const IntDateTime& intDateTime){
	this->date_ = intDateTime.date_;
	this->time_ = intDateTime.time_;
	return *this;
}

IntDateTime IntDateTime::operator=(const string& intDateTime){
	vector<string> vecDateTime = CStringManager::split(intDateTime, " ");
	if (vecDateTime.size() != 2){
		printf("解析时间出错");
		return *this;
	}
	string strDate = vecDateTime.at(0);
	string strTime = vecDateTime.at(1);
	CStringManager::Replace(strDate, "-", "");
	date_ = atoi(strDate.c_str());
	CStringManager::Replace(strTime, ":", "");
	time_ = atoi(strTime.c_str()) * 1000;
	return *this;
}

IntDateTime IntDateTime::operator=(const time_t& time){
	struct tm *p;
	//?取得当地时间
	p = localtime(&time);
	int year = 1900 + p->tm_year;
	int month = 1 + p->tm_mon;
	int day = p->tm_mday;
	int hour = p->tm_hour;
	int min = p->tm_min;
	int seconds = p->tm_sec;
	string strLocalDate;
	CStringManager::Format(strLocalDate, "%d%02d%02d", year, month, day);
	date_ = atoi(strLocalDate.c_str());
	string strLocalTime;
	CStringManager::Format(strLocalTime, "%02d%02d%02d000", hour, min, seconds);
	time_ = atoi(strLocalTime.c_str());
	return *this;
}

IntDateTime operator+(const IntDateTime& intDateTime, int seconds){
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

IntDateTime operator+(int seconds, const IntDateTime& intDateTime){
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

IntDateTime operator-(const IntDateTime& intDateTime, int seconds){
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

int operator-(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2){
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

bool operator< (const IntDateTime& intDateTime1, const IntDateTime& intDateTime2){
	if (intDateTime1.date_ < intDateTime2.date_) return true;
	else if (intDateTime1.date_ > intDateTime2.date_) return false;
	else{
		if (intDateTime1.time_ < intDateTime2.time_) return true;
		else if (intDateTime1.time_ > intDateTime2.time_) return false;
		else return false;
	}
}

bool operator> (const IntDateTime& intDateTime1, const IntDateTime& intDateTime2){
	if (intDateTime1.date_ < intDateTime2.date_) return false;
	else if (intDateTime1.date_ > intDateTime2.date_) return true;
	else{
		if (intDateTime1.time_ < intDateTime2.time_) return false;
		else if (intDateTime1.time_ > intDateTime2.time_) return true;
		else return false;
	}
}

bool operator<=(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2){
	if (intDateTime1.date_ < intDateTime2.date_) return true;
	else if (intDateTime1.date_ > intDateTime2.date_) return false;
	else{
		if (intDateTime1.time_ < intDateTime2.time_) return true;
		else if (intDateTime1.time_ > intDateTime2.time_) return false;
		else return true;
	}
}

bool operator>=(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2){
	if (intDateTime1.date_ < intDateTime2.date_) return false;
	else if (intDateTime1.date_ > intDateTime2.date_) return true;
	else{
		if (intDateTime1.time_ < intDateTime2.time_) return false;
		else if (intDateTime1.time_ > intDateTime2.time_) return true;
		else return true;
	}
}

bool operator==(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2){
	if (intDateTime1.date_ == intDateTime2.date_ && intDateTime1.time_ == intDateTime2.time_) return true;
	else return false;
}

bool operator!=(const IntDateTime& intDateTime1, const IntDateTime& intDateTime2){
	if (intDateTime1.date_ == intDateTime2.date_ && intDateTime1.time_ == intDateTime2.time_) return false;
	else return true;
}

bool operator==(const IntDateTime& intDateTime, int num){
	if (num != 0) return false;
	if (intDateTime.getDate() == 0 && intDateTime.getTime() == 0) return true;
	else return false;
}

bool operator==(int num, const IntDateTime& intDateTime){
	if (num != 0) return false;
	if (intDateTime.getDate() == 0 && intDateTime.getTime() == 0) return true;
	else return false;
}

void IntDateTime::setTime(int date, int time){
	date_ = date;
	time_ = time;
}

void IntDateTime::setTime(const string& time){
	vector<string> vecDateTime = CStringManager::split(time, " ");
	if (vecDateTime.size() != 2){
		printf("解析时间出错");
		return;
	}
	string strDate = vecDateTime.at(0);
	string strTime = vecDateTime.at(1);
	CStringManager::Replace(strDate, "-", "");
	date_ = atoi(strDate.c_str());
	CStringManager::Replace(strTime, ":", "");
	time_ = atoi(strTime.c_str()) * 1000;
}

void IntDateTime::setTime(time_t time){
	struct tm *p;
	//?取得当地时间
	p = localtime(&time);
	int year = 1900 + p->tm_year;
	int month = 1 + p->tm_mon;
	int day = p->tm_mday;
	int hour = p->tm_hour;
	int min = p->tm_min;
	int seconds = p->tm_sec;
	string strLocalDate;
	CStringManager::Format(strLocalDate, "%d%02d%02d", year, month, day);
	date_ = atoi(strLocalDate.c_str());
	string strLocalTime;
	CStringManager::Format(strLocalTime, "%02d%02d%02d000", hour, min, seconds);
	time_ = atoi(strLocalTime.c_str());
}

int IntDateTime::getDate()const{
	return date_;
}

int IntDateTime::getTime()const{
	return time_;
}

int IntDateTime::getYear()const{
	return date_ / 10000;
}

int IntDateTime::getMonth()const{
	return (date_ % 10000) / 100;
}

int IntDateTime::getDay()const{
	return (date_ % 10000) % 100;
}

int IntDateTime::getHour()const{
	return time_ / 10000000;
}

int IntDateTime::getMin()const{
	return (time_ % 10000000) / 100000;
}

int IntDateTime::getSeconds()const{
	return (time_ % 100000) / 1000;
}

string IntDateTime::toString(){
	string result;
	CStringManager::Format(result, "%d-%02d-%02d %02d:%02d:%02d", date_ / 10000, (date_ % 10000) / 100, (date_ % 10000) % 100, time_ / 10000000, (time_ % 10000000) / 100000, (time_ % 100000) / 1000);
	return result;
}

string IntDateTime::dateToString(){
	string result;
	CStringManager::Format(result, "%d-%02d-%02d", date_ / 10000, (date_ % 10000) / 100, (date_ % 10000) % 100);
	return result;
}

string IntDateTime::timeToString(){
	string result;
	CStringManager::Format(result, "%02d:%02d:%02d", time_ / 10000000, (time_ % 10000000) / 100000, (time_ % 100000) / 1000);
	return result;
}