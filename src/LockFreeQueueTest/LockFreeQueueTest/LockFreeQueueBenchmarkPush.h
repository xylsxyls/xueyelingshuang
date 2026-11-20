#pragma once

#include <cstdint>
#include <stdexcept>

// 共用负载适配 Queue 和标准库列表对照；仅用于测试，不进入生产头文件。
class LockFreeQueueBenchmarkPush
{
public:
    /** 执行普通 push 并检查接纳结果
    @param [in,out] queue 队列或绑定生产者
    @param [in] value 待入队值
    @return 0 成功；布尔接口失败为 -1（不属于库枚举）
    */
    template <class Queue, class Value>
    static int32_t submit(Queue& queue, const Value& value);

    /** 执行必须成功的基准路径；布尔失败抛 runtime_error
    @param [in,out] queue 队列或绑定生产者
    @param [in] value 待入队值
    */
    template <class Queue, class Value>
    static void checked(Queue& queue, const Value& value);

    /** 区分可由测试调用方重试的竞争/暂未安全复用；不吞掉真实分配和策略错误
    @param [in] result 被测接口的返回状态
    @return 本版有定义的临时未接纳状态；容量不足由各基准单独判断
    */
    static bool transient(int32_t result);

};

#include "LockFreeQueueBenchmarkPush.inl"