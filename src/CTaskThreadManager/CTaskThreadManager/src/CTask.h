#pragma once
#include "CTaskThreadManagerMacro.h"
#include <stdint.h>

class Semaphore;

//如果要获得任务执行结果，可以在创建task的时候就把获得结果的指针传入task内部，线程安全问题外部维护
/* 线程任务类
*/
class CTaskThreadManagerAPI CTask
{
	friend class CTaskThread;
public:
	//不要在构造和析构里做耗时操作，会降低发送任务的效率，但不会内存泄露或崩溃
	/* 构造函数
	*/
	CTask();
	
	/* 构造函数
	@param [in] taskId 任务ID，此ID号并非唯一ID，意思是一个类别，用于停止一个类别的任务
	*/
	CTask(int32_t taskId);

	/* 析构函数
	*/
	virtual ~CTask();

public:
	/* 线程中执行的任务函数
	*/
	virtual void DoTask() = 0;
	
	/* 中断任务
	*/
	virtual void StopTask();
	
	/* 当任务被更高优先级的任务顶掉后是否重做
	@return 返回是否重做
	*/
	virtual bool ReExecute();
	
	/* 如果重做会通过此函数构建一个新任务
	@return 返回任务指针
	*/
	virtual CTask* Clone();

	//此ID号并非唯一ID，意思是一个类别，用于停止一个类别的任务
	/* 获取任务ID
	@return 返回任务ID
	*/
	int32_t GetTaskId();

private:
	void SetWaitForSendHandle(Semaphore* waitForSend);
	
	Semaphore* GetWaitForSendHandle();

protected:
	//任务ID
	int32_t m_taskId;

	//SendTask中用到的阻塞事件
	Semaphore* m_waitForSend;
};
