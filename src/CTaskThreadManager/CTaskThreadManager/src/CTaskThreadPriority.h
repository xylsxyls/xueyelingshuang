#pragma once
#include "CTaskThreadModels.h"
#include <thread>

/** 系统优先级适配，调用者保证线程尚未 join 且句柄不被并发关闭
*/
class CTaskThreadPriority
{
public:
    /** 设置实际系统优先级，不改变任务优先级
    @param [in,out] thread 存活且可 join 的线程
    @param [in] priority 系统优先级
    @return 原生失败时返回系统错误码；不支持不冒充成功
    */
    static CTaskThreadResult apply(std::thread& thread, CTaskSystemPriority priority);
};