#pragma once
#include "CStopWatchMacro.h"

#define Create_Thread(FunName,ParameterName,ThreadID) CreateThread(NULL, 0, FunName, (LPVOID)ParameterName, 0, &ThreadID)

class CStopWatchAPI CStopWatch{
public:
	CStopWatch();
public:
	unsigned long time;
	unsigned long StopTime;
	unsigned long RunTime;
public:
	//凡是unsigned long的都是毫秒为单位，int的是分钟或者小时的整形，double是秒为单位

	//秒表是否在走
	bool StopOrRun;

	//时分秒是连一起的，如果是1小时1分1秒的时间，秒数返回的就是1秒，不是总秒数

	//查看走的小时数
	int GetHour();
	//查看走的分钟数
	int GetMinute();
	//查看走的秒数
	double GetSeconds();

	//查看走的毫秒数，总毫秒数
	unsigned long GetWatchTime();

	//设置秒表走的起始时间，传入的是总毫秒数
	void SetWatchTime(unsigned long SetTime);

	//停止走时间
	void Stop();

	//停止走一定的时间，过了这个时间执行虚函数并接着走，传入总毫秒数，如果在停止状态下执行此函数则以当前为准过一定秒数接着走并执行虚函数
	void Stop(unsigned long StopSecondsTime,void *pDo);
	virtual void Do(void *pDo);

	//继续走时间
	void Run();
};
