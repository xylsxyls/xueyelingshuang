#pragma once
#include "CTaskThreadModels.h"

/** 基础库状态统一为管理器的逐调用结果，不丢弃提交位
*/
class CTaskResultAdapter
{
public:
    /** 转换 FIFO 操作结果
    @param [in] status 基础库状态
    @return 管理器状态
    */
    static CTaskThreadResult queue(LockFreeQueueResult status);

    /** 转换有序表状态
    @param [in] status 基础库状态
    @return 管理器状态
    */
    static CTaskThreadResult map(LockFreeMapStatus status);

    /** 转换哈希表结果
    @param [in] result 基础库结果
    @return 管理器状态
    */
    static CTaskThreadResult hash(const LockFreeHashMapResult& result);

};