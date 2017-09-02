#pragma once
#include "CTaskThreadManagerMacro.h"
#include <stdint.h>
#include <windows.h>

//如果要获得任务执行结果，可以在创建task的时候就把获得结果的指针传入task内部，线程安全问题外部维护
class CTaskThreadManagerAPI CTask
{
	friend class CTaskThread;
public:
    CTask(int32_t taskId);

public:
    virtual void DoTask() = 0;
    virtual void StopTask(bool ifChoke) = 0;
    /* 当任务被优先级更高的任务顶掉时是否重做
    */
    virtual bool ReExecute() = 0;
    virtual CTask* Clone() = 0;
    virtual bool HasExecuted() = 0;
    int32_t GetTaskId();

private:
	void SetWaitForSendHandle(HANDLE waitForSend);
	HANDLE GetWaitForSendHandle();

private:
    int32_t m_taskId;

	/* SendTask中用到的阻塞事件
	*/
	HANDLE m_waitForSend = nullptr;
};