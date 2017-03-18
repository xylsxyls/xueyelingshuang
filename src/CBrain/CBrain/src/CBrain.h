#pragma once
#include "CBrainMacro.h"
#include <map>
#include <list>
#include <thread>
#include <mutex>
#include <atomic>
#include "CTask/CTaskAPI.h"
using namespace std;

class CBrainAPI CBrain{
public:
	enum{
		RUN,
		SUSPEND,
		END
	};

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	map<int, atomic<bool>> ctrl;
	map<int, list<CTask>> mapTask;
	list<CTask> listTaskBack;
	mutex mutex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	
	thread* workThread;
	thread* backThread;
	int currentLevel;
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
	//?恢复执行任务
	void RecoveryRun();
	//?结束执行任务
	void EndRun();
	//?工作线程
	void ThreadWork();
	//?后台线程
	void ThreadBack();
	//?删除前台执行，函数内部无锁
	void DeleteTask(const CTask& task);
	//?删除后台监测，函数内部无锁
	void DeleteBack(const CTask& task);
	//?取出最高优先级的任务，并将当前任务移动至当前优先级的队尾
	void GetCurrentTask();
	//?把任务添加进后台检测列表
	void AddBack(const CTask& task);
	//?移动到队尾
	void MoveToEnd(const CTask& task);
	//?执行任务，等待时间
	//?暂停任务，等待时间
	//?终止任务，等待时间
	//?后台监测，一旦触发任务条件则调该任务
};