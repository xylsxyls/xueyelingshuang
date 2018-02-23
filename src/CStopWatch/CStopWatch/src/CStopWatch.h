#pragma once
#include "CStopWatchMacro.h"
#include <windows.h>
#include <mutex>

#define Create_Thread(FunName,ParameterName,ThreadID) CreateThread(NULL, 0, FunName, (LPVOID)ParameterName, 0, &ThreadID)
#define CreateStopWatch(Type,Name) Type* Name = new Type

class std::mutex;
//typedef void* HANDLE;
class CStopWatchAPI CStopWatch{
public:
	CStopWatch();
	~CStopWatch();
public:
	unsigned long time;
	unsigned long StopTime;
	unsigned long RunTime;
public:
	//凡是unsigned long的都是毫秒为单位，int的是分钟或者小时的整形，double是秒为单位

	//秒表是否在走
	bool StopOrRun;

	//事件
	HANDLE handle;
	HANDLE ReturnHandle;
	unsigned long CountDownSeconds;
	std::mutex* pMutex;
	std::mutex* pMutexDo;

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

	//倒计时做一件事，再次调用则重置倒计时时间，该函数可以在多线程中调用，已加锁，但是必须保证线程运行的时候类存在
	void CountDown(unsigned long CountDownSeconds, void *pDo);
	//可以在虚函数内delete this，但是必须在delete前加上*nDelete = 1;
	virtual void Do(void *pDo, BOOL* nDelete);

	//继续走时间
	void Run();
};