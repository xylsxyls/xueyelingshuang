#pragma once
#include "CStopWatchMacro.h"
#include <stdint.h>

/** 倒计时任务基类指针
*/
class CountDownTask
{
public:
	virtual void DoTask() = 0;
};

class CStopWatchAPI CStopWatch
{
public:
	/** 构造函数
	*/
	CStopWatch();

	/** 析构函数
	*/
	~CStopWatch();

public:
	//时分秒是连一起的，如果是1小时1分1秒的时间，秒数返回的就是1秒，不是总秒数
	//查看走的小时数
	int32_t GetHour();

	//查看走的分钟数
	int32_t GetMinute();

	//查看走的秒数
	double GetSeconds();

	//查看走的毫秒数，总毫秒数
	unsigned long GetWatchTime();

	//设置秒表走的起始时间，传入的是总毫秒数
	void SetWatchTime(unsigned long SetTime);

	//继续走时间
	void Run();

	//停止走时间
	void Stop();

	//倒计时做一件事，再次调用则重置倒计时时间，该函数可以在多线程中调用，已加锁，如果threadId=0则创建，否则调用该ID线程
	static void CountDown(unsigned long countDownMsec, CountDownTask* task, int32_t taskId, int32_t& threadId);

	//放弃执行
	static void DiscardCountDown(int32_t taskId, int32_t threadId);

	//清空线程
	static void destroyThread(int32_t threadId);

private:
	//凡是unsigned long的都是毫秒为单位，int的是分钟或者小时的整形，double是秒为单位
	unsigned long m_time;
	unsigned long m_stopTime;
	unsigned long m_runTime;
	//秒表是否在走
	bool m_stopOrRun;
};