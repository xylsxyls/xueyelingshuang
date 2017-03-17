#pragma once
#include <string>
#include "CTaskMacro.h"
using namespace std;

class CTaskAPI CTask{
	//Run()和SuspendRun()必须进行重写
public:
	CTask();
public:
	//?UUID作为Task的唯一标识
	string uuid;
	//?任务流程节点，0表示初始化，1表示已经进入任务
	int node;
	//?是否启用检测异常任务
	bool ifExceptionRun;
	//?任务初始化
	void init(int level, bool ifExceptionRun = 0);
	//?检测当前状态是否可以执行
	virtual bool CheckRun();
	//?具体执行内容，返回1表示终止任务，后台检测线程一起关闭
	virtual bool Run();
	//?正常暂停任务
	virtual bool SuspendRun();
	//?异常处理，返回1表示终止任务
	virtual void ExceptionRun(int exception);
	//?后台检测异常，返回异常级别，不可以阻塞，不能死循环
	virtual int CheckException();
};