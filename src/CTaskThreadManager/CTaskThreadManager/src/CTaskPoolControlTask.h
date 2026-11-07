#pragma once
#include "CTask.h"

class CTaskPoolState;

/** 控制任务只负责评估扩缩容；所有业务工作仍由管理器线程执行
*/
class CTaskPoolControlTask : public CTask
{
public:
    /** 借用线程池状态
    @param [in,out] state 必须存活到控制线程完成 join
    */
    explicit CTaskPoolControlTask(CTaskPoolState& state);

    /** 周期执行策略直到请求退出
    */
    virtual void DoTask();

    /** 发布停止意图并唤醒，不等待自身
    */
    virtual void StopTask();

private:
    // 生命周期由池门面保证
    CTaskPoolState* m_state;
};