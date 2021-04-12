#pragma once
#include "CTaskThreadManagerMacro.h"
#include <stdint.h>

class Semaphore;
//如果要获得任务执行结果，可以在创建task的时候就把获得结果的指针传入task内部，线程安全问题外部维护
class CTaskThreadManagerAPI CTask
{
	friend class CTaskThread;
public:
	//不要在构造和析构里做耗时操作，会降低发送任务的效率，但不会内存泄露或崩溃
	CTask();
    CTask(int32_t taskId);

	virtual ~CTask();

public:
    virtual void DoTask() = 0;
	//当需要中断时加上
    virtual void StopTask();
	//当需要顶掉后重做逻辑时加上
    virtual bool ReExecute();
    virtual CTask* Clone();

	//此ID号并非唯一ID，意思是一个类别，用于停止一个类别的任务
    int32_t GetTaskId();

private:
	void SetWaitForSendHandle(Semaphore* waitForSend);
	Semaphore* GetWaitForSendHandle();

protected:
    int32_t m_taskId;

	/* SendTask中用到的阻塞事件
	*/
	Semaphore* m_waitForSend;
};