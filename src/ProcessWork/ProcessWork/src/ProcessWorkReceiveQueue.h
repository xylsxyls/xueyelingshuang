#pragma once
#include "ProcessWorkMemory.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include <atomic>
#include <memory>
#include <vector>

/** 唯一读取线程填充、唯一回调线程消费的连续正文批次。
*/
struct ProcessWorkReceiveBatch
{
public:
    // 批次独占的连续记录内存
    char* m_data;
    // 实际可写容量
    size_t m_capacity;
    // 已提交记录区长度；发布后生产者不再修改
    size_t m_size;

public:
    /** 申请一个固定容量批次；失败时m_data为空，由队列申请入口报告资源不足
    @param [in] capacity 正文记录区字节数
    */
    explicit ProcessWorkReceiveBatch(size_t capacity);

    /** 释放独占记录区。
    */
    ~ProcessWorkReceiveBatch();

private:
    /** 禁止复制独占正文。
    @param [in] other 不允许复制的批次
    */
    ProcessWorkReceiveBatch(const ProcessWorkReceiveBatch& other);

    /** 禁止复制覆盖独占正文。
    @param [in] other 不允许复制的批次
    @return 不可调用
    */
    ProcessWorkReceiveBatch& operator=(const ProcessWorkReceiveBatch& other);
};

/** 接收端SPSC批次队列。每包仍独立回调和计费，批次只减少分配与线程间发布次数。
    append/flush仅共享读取线程调用；take/release仅回调线程调用；configure/clear在静止期。
*/
class ProcessWorkAPI ProcessWorkReceiveQueue
{
public:
    /** 建立空队列，正文块按需申请。
    */
    ProcessWorkReceiveQueue();

    /** 两个使用线程已停止后清理全部独占资源。
    */
    ~ProcessWorkReceiveQueue();

    /** 静止且无在途正文时设置预算。
    @param [in] config 系统内存保护和正文预算
    @return true设置成功，false参数无效或存在正文
    */
    bool configure(const ProcessWorkMemoryConfig& config);

    /** 复制一条完整共享消息到本地批次，成功后共享槽可立即复用。
    @param [in] source 借用的12字节包头及正文
    @param [in] bytes 包头和正文长度，不包含自动追加的NUL
    @return true已拥有完整副本；false未接纳，调用方保留共享正文
    */
    bool append(const char* source, size_t bytes);

    /** 发布已复制的批次，失败仍保留副本供后续重试，绝不重新复制已接纳消息。
    @return true当前没有未发布批次，false队列资源暂不可用
    */
    bool flush();

    /** 取下一条独立正文，借用至对应release；同一时刻只允许一条借用。
    @param [out] packet 包头和正文指针
    @param [in] milliseconds 空队列最长等待毫秒
    @return true有消息，false本轮无消息
    */
    bool take(char** packet, uint32_t milliseconds);

    /** 当前回调完成后归还这一条正文的预算，最后一条归还批次。
    @param [in] packet 最近take返回的地址
    */
    void release(char* packet);

    /** 唤醒回调线程以观察停止或满载事件。
    */
    void wake();

    /** 静止时释放未处理正文与缓存批次，重置计费。
    */
    void clear();

    /** 查询预留、排队及正在回调的正文计费快照。
    @return 有限额度下尚未归还的字节数；-1模式不计费并返回0，不能据此判断为空
    */
    uint64_t used() const;

    /** 观察尚未完成的消息，包含未发布及正在回调的正文
    @return 弱一致存活快照，静止期用于配置保护
    */
    bool hasPending() const;

    /** 借用回调通知后端，满载事件也通过它唤醒。
    @return 本对象拥有的通知
    */
    Semaphore& notification();

private:
    /** 取得足够容量的缓存块或申请新块，不等待其他线程。
    @param [in] bytes 本次记录需要的容量
    @return 独占批次或nullptr
    */
    ProcessWorkReceiveBatch* acquire(size_t bytes);

    /** 回调线程归还空批次；仅保留有限个标准大小的块。
    @param [in] batch 已完整消费且未发布到其他位置的批次
    */
    void recycle(ProcessWorkReceiveBatch* batch);

    /** 禁止复制资源所有权。
    @param [in] other 不允许复制的来源
    */
    ProcessWorkReceiveQueue(const ProcessWorkReceiveQueue& other);

    /** 禁止复制资源所有权。
    @param [in] other 不允许复制的来源
    @return 不可调用
    */
    ProcessWorkReceiveQueue& operator=(const ProcessWorkReceiveQueue& other);

    /** 销毁一个独占批次并归还实际分配预算。
    @param [in] batch 待销毁的批次，允许nullptr
    */
    void destroy(ProcessWorkReceiveBatch* batch);

private:
    // 唯一读取线程独占的未发布批次
    ProcessWorkReceiveBatch* m_building;
    // 预先准备但尚未投入周转的块，仅由读取线程领取；优先复用m_free中的热块
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::vector<ProcessWorkReceiveBatch*> m_unused;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 静止配置的标准块容量；超过它的单条消息使用一次性块
    size_t m_batchBytes;
    // 有限额度累计接纳字节，-1模式仅累计消息数；允许无符号回绕
    uint64_t m_reserved;
    // 读取线程缓存的完成进度，仅在预算将满时刷新，避免逐包读取消费端缓存行
    uint64_t m_knownReleased;
    // 申请失败后的单调时间退避，仅读取线程修改
    uint64_t m_nextMemoryRetry;
    // 不同写线程的计费发布在独立缓存行，常态不做共享读改写
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<uint64_t> m_charged;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 不依赖堆分配的超额对齐，确保两组独占可写状态至少相隔一个缓存行
    char m_producerPadding[64];
    // 唯一回调线程正在借用的批次
    ProcessWorkReceiveBatch* m_active;
    // 回调线程的批次内读取位置
    size_t m_readOffset;
    // 与接纳端同单位的累计完成值，-1模式仅累计完成消息数
    uint64_t m_completed;
    // 每条回调完成后的累计计费发布
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<uint64_t> m_released;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 将回调端独占写状态与队列控制字段分开
    char m_consumerPadding[64];
    // 已完成正文批次的SPSC队列
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueue<ProcessWorkReceiveBatch*> m_ready;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 回调端归还空批次的反向SPSC队列
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueue<ProcessWorkReceiveBatch*> m_free;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 缓存标准块，限额为max(8,配置的初始预分配块数)，不随峰值无限增长
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<uint32_t> m_cached;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 含正在填充、排队、回调和缓存块的实际记录区分配量，避免稀疏批次放大内存
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<uint64_t> m_allocatedBytes;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 合并数据通知，非消息许可计数
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<bool> m_notified;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 消费线程空闲时休眠，通知本身不代表可消费条数
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    Semaphore m_notification;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 运行期间只读的内存保护参数
    ProcessWorkMemoryConfig m_config;
    // 实际批次新增次数，不含初始化预留，也不含空闲批次复用
    uint64_t m_storageGrows;
};

#include "ProcessWorkReceiveQueue.inl"