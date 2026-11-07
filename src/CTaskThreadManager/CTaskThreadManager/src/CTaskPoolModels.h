#pragma once
#include "CTaskThreadModels.h"

class CTaskPoolPolicy;

/** 策略输入，全部为本轮观察值，不承担并发容量预留
*/
struct CTaskThreadManagerAPI CTaskPoolLoad
{
public:
    // 初始线程数，也是缩容下限
    size_t m_initialThreads;
    // 当前注册工作线程数，不包含控制线程
    size_t m_currentThreads;
    // 初始化后成功新增线程次数
    uint64_t m_successfulGrowCount;
    // 等待执行的任务数
    uint64_t m_queuedTasks;
    // 正在执行业务的线程数
    size_t m_runningThreads;
    // 没有执行和排队任务的线程数
    size_t m_idleThreads;
    // 全池连续无任务的单调毫秒数
    uint64_t m_idleMilliseconds;
    // 距最近一次成功调整的毫秒数
    uint64_t m_sinceResizeMilliseconds;

public:
    /** 构造零负载
    */
    CTaskPoolLoad();

};

/** 扩缩容算法的请求；线程池负责上限、下限和在途任务保护
*/
struct CTaskThreadManagerAPI CTaskPoolDecision
{
public:
    // 期望总线程数；由线程池截断到初始值和最大值之间
    size_t m_targetThreads;
    // 缩容所需连续空闲时间，0 表示本次不缩容
    uint64_t m_shrinkIdleMilliseconds;

public:
    /** 构造决策
    @param [in] target 期望总数
    @param [in] idleMilliseconds 缩容空闲要求
    */
    CTaskPoolDecision(size_t target = 0, uint64_t idleMilliseconds = 0);

};

/** 初始化时冻结的线程池配置，动态判断放在独立策略对象
*/
struct CTaskThreadManagerAPI CTaskThreadPoolConfig
{
public:
    // 初始化工作线程数，不包含控制线程
    size_t m_initialThreads;
    // 工作线程上限，-1 不设配置上限
    int64_t m_maximumThreads;
    // 自动评估周期，毫秒，必须为正
    uint32_t m_checkMilliseconds;
    // 新增工作线程统一使用的参数
    CTaskThreadConfig m_worker;
    // 线程池成员索引的节点池
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
    LockFreeMapConfig m_members;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 可继承的扩缩容算法，空指针使用默认策略；保存强引用
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
    std::shared_ptr<CTaskPoolPolicy> m_policy;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

public:
    /** 默认 2 个线程，最多 64，每 100 毫秒评估
    */
    CTaskThreadPoolConfig();

};

/** 线程池的弱一致统计
*/
struct CTaskThreadManagerAPI CTaskPoolStatistics
{
public:
    // 当前成员数量，不包含控制线程
    size_t m_threads;
    // 成功扩容次数
    uint64_t m_grows;
    // 成功缩容次数
    uint64_t m_shrinks;
    // 策略异常、初始化、收尾或资源错误次数
    uint64_t m_errors;
    // 是否允许选择新目标
    bool m_accepting;

public:
    /** 初始化空统计
    */
    CTaskPoolStatistics();

};