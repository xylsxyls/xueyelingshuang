#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>

class LumaPlayerLogicController;

/** 生命周期执行任务，与GUI和逻辑线程分离；收尾开始后必须完整完成
*/
class LumaPlayerLifecycleTask : public CTask
{
public:
    /** 保存生命周期操作
    @param [in] controller 借用控制器，外层等待本线程结束后释放
    @param [in] shutdown true表示按依赖关闭，false表示后台初始化
    */
    LumaPlayerLifecycleTask(LumaPlayerLogicController* controller, bool shutdown);

    /** 执行初始化或完整有序回收，不在自身线程Uninit自身
    */
    virtual void DoTask();

    /** 请求中断初始化，关闭任务仍必须完成资源回收
    */
    virtual void StopTask();

private:
    // 借用控制器
    LumaPlayerLogicController* m_controller;
    // 操作模式
    bool m_shutdown;
    // 协作取消标记
    std::atomic<bool> m_exit;
};