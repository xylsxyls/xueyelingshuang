#pragma once
#include "LumaPlayerLogicTask.h"

/** 独立业务任务，继承统一取消标记，控制器在逻辑线程停止后释放
*/
class LumaPlayerLoadTask : public LumaPlayerLogicTask
{
public:
    /** 复制本次业务输入
    @param [in] controller 借用的逻辑控制器
    @param [in] action 输入副本
    */
    LumaPlayerLoadTask(LumaPlayerLogicController* controller, const LumaPlayerLogicAction& action);

    /** 取消检查后完成一次短投递段，不等待解码或GUI
    */
    virtual void DoTask();
};