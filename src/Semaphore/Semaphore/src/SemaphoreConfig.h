#pragma once
#include "SemaphoreEventPool.h"

/** 等待和重试的实例参数，所有时间单位均为毫秒。通过setWaitPolicy验证后在构造时生效。
*/
struct SemaphoreAPI SemaphoreWaitPolicy
{
public:
    // 普通计数等待多久后转入优先交付，0表示直接登记。
    uint32_t m_promotionMilliseconds;
    // 本地等待核对谓词的最长间隔，必须大于0且小于UINT32_MAX。
    uint32_t m_localSlice;
    // 移动共享计数补查间隔，1至999。
    uint32_t m_mobileSlice;
    // POSIX有限等待补查间隔，1至999。
    uint32_t m_posixSlice;
    // 满载后允许先快速重试的次数，0表示直接退让。
    uint32_t m_capacityAttempts;
    // 用尽快速重试次数后的睡眠时间，必须大于0且小于UINT32_MAX。
    uint32_t m_capacitySleep;
    // 连续竞争多少轮后让出执行时间片，正数。
    uint32_t m_contentionYieldInterval;
    // x86首次CAS竞争退避使用的PAUSE数，0可禁用处理器提示。
    uint32_t m_initialPauses;
    // x86单轮PAUSE上限，范围为initialPauses至1024；其他架构只执行周期退让。
    uint32_t m_maximumPauses;

public:
    /** 设置与普通构造一致的默认时间和重试预算
    */
    SemaphoreWaitPolicy();
};

/** 构造前独立配置两个池，构造完成后可以销毁配置。池回调捕获对象须满足自己的生命周期。
*/
class SemaphoreAPI SemaphoreConfig
{
public:
    /** 默认使用惰性建池和默认等待策略
    */
    SemaphoreConfig();

    /** 选择预建事件池并返回可修改的配置
    @return 当前配置持有的事件池参数
    */
    SemaphorePoolConfig& configureEventPool();

    /** 选择预建计数池并返回可修改的配置
    @return 当前配置持有的计数池参数
    */
    SemaphorePoolConfig& configureCountPool();

    /** 设置本实例的等待策略，非法参数保留原值
    @param [in] policy 完整策略
    @return OK或INVALID
    */
    SemaphoreResult setWaitPolicy(const SemaphoreWaitPolicy& policy);

    /** 读取待应用的策略
    @return 与该配置同寿命的只读策略
    */
    const SemaphoreWaitPolicy& waitPolicy() const;

    /** 查询是否显式请求预建事件池
    @return 配置拥有的池参数，未请求时为空
    */
    const SemaphorePoolConfig* eventPool() const;

    /** 查询是否显式请求预建计数池
    @return 配置拥有的池参数，未请求时为空
    */
    const SemaphorePoolConfig* countPool() const;

private:
    // 构造时复制进实例的等待策略。
    SemaphoreWaitPolicy m_policy;
    // 本配置持有的事件池参数。
    SemaphorePoolConfig m_event;
    // 计数等待和极大计数记录使用的配置。
    SemaphorePoolConfig m_count;
    // 是否要求构造时预建事件池。
    bool m_prepareEvent;
    // 是否要求构造时预建计数池。
    bool m_prepareCount;
};