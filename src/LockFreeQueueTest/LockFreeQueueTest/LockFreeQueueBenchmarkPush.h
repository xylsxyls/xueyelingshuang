#pragma once

#include <cstdint>
#include <type_traits>
#include <stdexcept>

// 相同负载适配旧 void push 与当前状态返回接口；仅用于测试，不进入生产头文件。
class LockFreeQueueBenchmarkPush
{
public:
    /** 执行对应版本的普通 push，并保留其真实失败状态
    @param [in,out] queue 队列或绑定生产者
    @param [in] value 待入队值
    @return 0 成功；其他数值与该版本 LockFreeQueueResult 一致
    */
    template <class Queue, class Value>
    static int32_t submit(Queue& queue, const Value& value);

    /** 执行必须成功的基准准备或数据路径；失败交由测试外层记录
    @param [in,out] queue 队列或绑定生产者
    @param [in] value 待入队值
    */
    template <class Queue, class Value>
    static void checked(Queue& queue, const Value& value);

    /** 区分可由测试调用方重试的竞争/暂未安全复用；不吞掉真实分配和策略错误
    @param [in] result 被测版本的返回状态
    @return 本版有定义的临时未接纳状态；容量不足由各基准单独判断
    */
    static bool transient(int32_t result);

private:
    /** 仅在被测枚举确有资源占用码时比较它；旧基线不虚构新状态
    @param [in] result 被测返回值
    @param [in] selected 编译期重载选择
    @return 是否为该版本资源占用码
    */
    template <class Result>
    static auto resourceUnavailable(int32_t result, int selected)
        -> decltype(Result::LockFreeQueueResourceUnavailable, bool());

    /** 旧版枚举没有独立资源码时选用此重载
    @param [in] result 被测返回值
    @param [in] selected 编译期回退选择
    @return false
    */
    template <class Result>
    static bool resourceUnavailable(int32_t result, long selected);

    /** 适配旧 void 接口，捕获该版本已有的资源错误
    @param [in,out] queue 队列或绑定生产者
    @param [in] value 待入队值
    @param [in] legacy 编译期旧签名标记
    @return 入队状态
    */
    template <class Queue, class Value>
    static int32_t execute(Queue& queue, const Value& value, std::true_type legacy);

    /** 直接返回当前状态接口的结果，不增加异常转换
    @param [in,out] queue 队列或绑定生产者
    @param [in] value 待入队值
    @param [in] legacy 编译期当前签名标记
    @return 入队状态
    */
    template <class Queue, class Value>
    static int32_t execute(Queue& queue, const Value& value, std::false_type legacy);
};

#include "LockFreeQueueBenchmarkPush.inl"