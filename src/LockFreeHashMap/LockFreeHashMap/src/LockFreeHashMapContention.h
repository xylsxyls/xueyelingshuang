#pragma once
#include <stdint.h>

/** 单次操作的竞争退让，不持有互斥权，也不等待指定线程完成
*/
class LockFreeHashMapContention
{
public:
    /** 构造尚未发生竞争的局部状态
    */
    LockFreeHashMapContention();

    /** 连续竞争时让出运行机会；不撤销请求，不改变线程优先级
    */
    void pause() throw();

    /** 主动让出一次运行机会；调用方管理借用范围，不等待指定线程，也不改变优先级
    */
    static void yieldExecution() throw();

private:
    /** 把调度及抖动隔离到冷路径，避免扩大普通查找和 CAS 热路径的内联代码
    */
    void reschedule() throw();
private:
    // 有界竞争计数，第一次一百二十八次失败后退让，后续间隔为六十五至一百二十八次
    uint32_t m_attempts;
    // 按操作地址播种的局部抖动，避免多个败者同步重试
    uint32_t m_jitter;
};

#include "LockFreeHashMapContention.inl"