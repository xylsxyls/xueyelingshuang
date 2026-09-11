#pragma once
#include "LumaPlayerLogicModels.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>

class LumaPlayerLogicController;

/** 单个短逻辑任务，只调度，不等待Core或GUI
*/
class LumaPlayerLogicTask : public CTask
{
public:
    /** 复制输入；控制器活到该线程回收结束
    @param [in] controller 借用控制器
    @param [in] action 输入副本
    */
    LumaPlayerLogicTask(LumaPlayerLogicController* controller, const LumaPlayerLogicAction& action);

    /** 检查取消后处理短动作；一次状态提交段完整完成
    */
    virtual void DoTask();

    /** 标记取消，不等待
    */
    virtual void StopTask();

protected:
    // 借用控制器
    LumaPlayerLogicController* m_controller;
    // 输入副本
    LumaPlayerLogicAction m_action;
    // 退出标记
    std::atomic<bool> m_exit;
};