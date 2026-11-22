#pragma once
#include "CTaskThreadModels.h"
#include "CTaskAdmission.h"
#include "CTaskStorageModels.h"

/** 一个优先级的 MPSC FIFO；取出、重做槽和维护只由工作线程访问
*/
class CTaskPriorityBucket
{
public:
    /** 建立空桶
    @param [in] level 优先级
    */
    explicit CTaskPriorityBucket(int32_t level);

    /** 对静止的未启动队列应用并校验参数，不建立节点池。
    @param [in] config 本级队列参数
    @return 基础库配置结果
    */
    CTaskThreadResult configure(const CTaskQueueConfig& config);

    /** 静止期配置队列
    @param [in] config 每线程配置
    @return 初始化结果
    */
    CTaskThreadResult initialize(const CTaskThreadConfig& config);

    /** 唯一消费者从混合模式队列读取一个值前缀，保存稳定记录供逐项选级
    调用前已消费完上次暂存的前缀；未观察到已发布项时有效项数为零
    */
    void refillValuePrefix();

    /** 为可回收桶预留一项，队列从空变为非空时同时持有一份退役保护。
    调用者须已登记为生产者，或是正在提交重做的唯一消费者。
    */
    void reserveQueued();

    /** 归还可回收桶的一项预留；最后一次取走或投递回滚时解除退役保护。
    */
    void releaseQueued();

public:
    // 不变优先级
    const int32_t m_level;
    // 建桶时从本级配置复制；仅专用模式支持按 Entry 地址读取前缀
    bool m_entryOnly;
    // 下标小于该值的读侧缓存可长期绑定生产者，至少一个登记位留给逐次投递。
    size_t m_cachedProducerLimit;
    // 退役门计入在途生产者；可回收桶有排队预留时还持有一份登记
    CTaskAdmission m_writers;
    // 接纳计数不与 FIFO 的首个缓存行共用
    char m_writerPadding[64];
    // 单消费者队列
    LockFreeQueue<CTaskStorageSlot*> m_queue;
    // 仅消费者访问，优先于本级 FIFO；重做不重新分配队列节点
    CTaskStorageSlot* m_redo;
    // 消费者本地前缀的下一个位置；仅在选定任务时推进，观察队首不消费
    size_t m_readyHead;
    // 消费者本地前缀的有效项数，尚未取出的范围为 [m_readyHead, m_readyCount)
    size_t m_readyCount;
    // 最多预读 256 个记录基类地址，合并归还 FIFO 额度；记录仍计入原优先级排队数
    LockFreeQueueEntry<CTaskStorageSlot*>* m_readyEntries[256];
    // 消费者独占引用与生产者可写计数隔离
    char m_frontPadding[64];
    // 排队数量，包含正在发布的预留和重做槽
    std::atomic<uint64_t> m_queued;
    // 生产者累计量不与消费者取出量争用缓存行
    char m_countPadding[64];
    // 常用桶只由消费者写，和各生产者的分级累计量作模差
    std::atomic<uint64_t> m_taken;
    // 隔离消费者写入的取出计数
    char m_readyPadding[64];

};