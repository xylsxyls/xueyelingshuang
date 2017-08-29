#pragma once
#include "CTaskThreadManagerMacro.h"

//如果要获得任务执行结果，可以在创建task的时候就把获得结果的指针传入task内部，线程安全问题外部维护
class CTaskThreadManagerAPI CTask
{
public:
    CTask(__int32 taskId);

public:
    virtual void DoTask() = 0;
    virtual void StopTask(bool ifChoke) = 0;
    /* 当任务被优先级更高的任务顶掉时是否重做
    */
    virtual bool ReExecute() = 0;
    virtual CTask* Clone() = 0;
    virtual bool HasExecuted() = 0;
    __int32 GetTaskId();

private:
    __int32 m_taskId;
};