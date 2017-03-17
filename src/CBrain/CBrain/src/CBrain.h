#pragma once
#include "CBrainMacro.h"
#include <map>
#include <list>
#include <thread>
#include <mutex>
#include <atomic>
#include "CTask/CTaskAPI.h"
using namespace std;

class CBrain{
public:
	enum{
		RUN,
		SUSPEND,
		END
	};
	map<int, atomic<bool>> ctrl;
	map<int, list<CTask>> mapTask;
	list<CTask> listTaskBack;
	mutex mutex;
	thread* workThread;
	thread* backThread;
	CTask currentTask;
public:
	CBrain();
	//?监听信息，始终并行，初始化，监听设置执行暂停终止，1开始0暂停-1终止
	//void listen(int flag = 1);
	//?任务列表，level为任务优先级，0为最高
	void push_back(const CTask& task, int level);
	//?开始执行任务
	void Run();
	//?暂停执行任务
	void SuspendRun();
	//?结束执行任务
	void EndRun();
	//?工作线程
	void ThreadWork();
	//?后台线程
	void ThreadBack();
	//?执行任务，等待时间
	//?暂停任务，等待时间
	//?终止任务，等待时间
	//?后台监测，一旦触发任务条件则调该任务
};