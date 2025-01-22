#pragma once
#include "CStopWatchMacro.h"
#include <stdint.h>
#include <string>

class CStopWatchAPI CStopWatch
{
public:
	/** 构造函数
	*/
	CStopWatch();

public:
	//时分秒是连一起的，如果是1小时1分1秒的时间，秒数返回的就是1秒，不是总秒数
	//查看走的小时数
	int32_t GetHour();

	//查看走的分钟数
	int32_t GetMinute();

	//查看走的秒数
	double GetSeconds();

	//查看走的总时间
	std::string GetWatchStrTime();

	//查看走的毫秒数，总毫秒数
	unsigned long GetWatchTime();

	//设置秒表走的起始时间，传入的是总毫秒数
	void SetWatchTime(unsigned long SetTime);

	//继续走时间
	void Run();

	//停止走时间
	void Stop();

protected:
	//获取开机以来的时间
	unsigned long GetOpenTime();

private:
	//凡是unsigned long的都是毫秒为单位，int的是分钟或者小时的整形，double是秒为单位
	unsigned long m_time;
	unsigned long m_stopTime;
	unsigned long m_runTime;
	//秒表是否在走
	bool m_stopOrRun;
};