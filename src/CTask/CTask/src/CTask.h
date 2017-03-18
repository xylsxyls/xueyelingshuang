#pragma once
#include <string>
#include "CTaskMacro.h"
using namespace std;

class CTaskAPI CTask{
	//Run()和SuspendRun()必须进行重写
public:
	CTask();
public:
	enum{
		INITSUCCESS = 1,
		RUNSUCCESS = 1,
		RUNBACK = 0,
		RUNSUSPEND = 2
	};
public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//?UUID作为Task的唯一标识
	string uuid;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	
	//?任务流程节点，-1表示空任务，0表示初始化，1表示已经进入任务
	int node;
	//?是否启用检测异常任务
	bool ifExceptionRun;
	//?任务初始化
	void init(int level, bool ifExceptionRun = 0);
	//?检测是否可以运行以及初始化当前节点，返回1表示可以运行
	virtual int initRun();
	//?具体执行内容，返回RUNSUCCESS表示终止任务，后台检测线程一起关闭，返回RUNBACK表示后台检测一直运行，返回RUNSUSPEND表示手动正常暂停
	virtual int Run();
	//?正常暂停任务，任务被正常暂停之后Run函数返回0，下次判断此任务是否是暂停住的，如果是先进入恢复任务然后根据节点进入上次暂停的地方
	virtual void SuspendRun();
	//?异常处理，传入-1表示CheckRun错误，返回1表示终止任务
	virtual void ExceptionRun(int exception);
	//?后台检测异常，返回异常级别，不可以阻塞，不能死循环
	virtual int CheckException();
	//?设置为空任务
	void SetEmpty();
	//?检测该任务是否有效
	bool isValid();
};