#pragma once
#include "ProcessWorkMacro.h"
#include "ProcessWorkMemoryPolicy.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <stdint.h>
#include <memory>

/** 内存压力保护，独立于业务配置的共享缓冲块数上限。
*/
struct ProcessWorkAPI ProcessWorkMemoryConfig
{
public:
    // 给系统保留的可用物理/提交内存字节，默认256MiB。
    uint64_t m_reserveBytes;
    // 同时保留的物理内存比例，默认10；取两种保留量的较大值，范围0..90。
    uint32_t m_reservePercent;
    // 单轮扩容字节预算，默认4MiB；单块更大时一轮只创建一块。
    uint64_t m_growthBytes;
    // 申请失败后最短重试间隔毫秒，默认100，必须大于0。
    uint32_t m_retryMilliseconds;
    // 本地积压固定字节上限，正数不增长；默认-1不限制且跳过逐消息字节计费，0和小于-1非法
    int64_t m_localQueueBytes;
    // 本地队列初始节点数，默认64；静止初始化时准备，不代表消息正文预算
    uint32_t m_localQueueInitialCapacity;
    // 接收端初始化时预分配的标准批次数，默认0按需申请；空闲备用块不占用消息积压额度
    uint32_t m_receiveBatchReserve;
    // Post小包备用槽数，默认0；每槽64字节，正文含内部头和计费前缀至多60字节
    uint32_t m_postBufferReserve;
    // Post小包缓存池最大槽数，-1无配置上限，0关闭缓存池；大包及缓存未命中仍受本地字节预算约束
    int64_t m_postBufferMaximum;
    // 本地就绪队列完整配置；旧initial参数非64时覆盖这里的初始容量，冲突配置拒绝
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueuePoolConfig m_queuePool;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 接收空闲批次队列的独立配置，Post端不使用
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueuePoolConfig m_freeQueuePool;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 就绪队列的增长算法，空函数保留基础库默认算法
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueueGrowPolicy m_queueGrow;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 就绪队列的缩容算法，空函数保留基础库默认策略
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueueShrinkPolicy m_queueShrink;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 接收空闲队列的独立增长算法
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueueGrowPolicy m_freeQueueGrow;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 接收空闲队列的独立缩容算法
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueueShrinkPolicy m_freeQueueShrink;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 实际正文池和共享块增长机制；不调整积压额度，空指针使用默认算法，初始化复制强引用
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::shared_ptr<ProcessWorkMemoryPolicy> m_policy;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

public:
    /** 设置默认保护，不设置业务容量上限。
    */
    ProcessWorkMemoryConfig();

    /** 初始化前按MB设置固定积压额度，1MB=1024*1024字节；不会调整运行中实例
    @param [in] megabytes 正整数MB，-1关闭额度限制；0、其他负数及换算溢出均非法
    @return true已更新本配置的m_localQueueBytes，false保持原配置不变
    */
    bool setLocalQueueLimitMB(int64_t megabytes);
};

/** 查询系统资源后再创建共享映射；查询失败时拒绝扩容，不循环盲试。
*/
class ProcessWorkAPI ProcessWorkMemory
{
public:
    /** 检查策略参数，不分配共享内存。
    @param [in] config 内存保护参数
    @return true参数有效
    */
    static bool valid(const ProcessWorkMemoryConfig& config);

    /** 检查本次新增字节是否仍留有系统余量；不是跨进程资源预留。
    @param [in] config 内存保护参数
    @param [in] bytes 本次最坏新增量，包含映射及元数据
    @return true允许尝试，false查询失败或压力过高
    */
    static bool allows(const ProcessWorkMemoryConfig& config, uint64_t bytes);

    /** 合并旧初始容量参数与新的完整队列配置，检查冲突
    @param [in] config 初始化配置
    @param [in] freeQueue true选择接收空闲批次队列
    @param [out] pool 合并后的配置
    @return true参数一致，false初始容量冲突
    */
    static bool queueConfig(const ProcessWorkMemoryConfig& config, bool freeQueue, LockFreeQueuePoolConfig& pool);
};