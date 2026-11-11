#pragma once
#include "ProcessWorkMemory.h"
#include "ProcessWorkPacketPool.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include <atomic>

/** 带正文计费和合并唤醒的本地MPSC队列；唯一消费者完成处理后归还正文。
    队列、正在处理和预留中的正文共用预算，避免慢回调将共享内存背压转化为无限堆积压。
*/
class ProcessWorkAPI ProcessWorkLocalQueue
{
public:
    /** 建立默认队列和内存保护参数。
    */
    ProcessWorkLocalQueue();

    /** 外部使用者全部停止之后清理尚未消费的正文。
    */
    ~ProcessWorkLocalQueue();

    /** 静止且无在途正文时配置计费预算与初始队列；参数失败不改变配置，池准备失败须重试初始化
    @param [in] config 系统余量及本地正文预算
    @return true成功，false参数无效、仍有在途正文或池准备失败
    */
    bool configure(const ProcessWorkMemoryConfig& config);

    /** 预留计费并申请正文，失败不会占用预算。
    @param [in] bytes 正文字节数，不包含内部计费前缀
    @param [out] result 失败原因，成功为LockFreeQueueSuccess
    @return 正文地址；由本类release释放，不能直接delete[]
    */
    char* allocate(size_t bytes, LockFreeQueueResult& result);

    /** 预留预算并申请正文，普通post可持续处理预算CAS竞争
    @param [in] bytes 正文字节数
    @param [out] result 成功或明确的未提交失败原因
    @param [in] retryContention true不限制CAS竞争重试；不重试容量和系统资源不足
    @return 正文地址或nullptr，成功后由release归还
    */
    char* allocate(size_t bytes, LockFreeQueueResult& result, bool retryContention);

    /** 释放从未提交或已经由唯一消费者取出的正文。
    @param [in] packet allocate返回的地址，nullptr无操作
    */
    void release(char* packet);

    /** 归还独占持有的一组正文，合并预算扣减；每个非空指针必须来自本队列且只出现一次
    @param [in,out] packets 至少count个位置；归还后逐项置空，nullptr无操作
    @param [in] count 指针位置数量
    */
    void releasePrefix(char** packets, size_t count);

    /** 提交完整正文并合并通知，成功之后调用者不再访问packet。
    @param [in] packet 本类已分配的完整正文
    @return Success已提交，其他状态未提交且调用者仍负责release
    */
    LockFreeQueueResult submit(char* packet);

    /** 唯一消费者尝试取出正文；空队列登记睡眠后复查，避免丢通知。
    @param [out] packet 取出的正文所有权
    @param [in] milliseconds 空队列最多等待毫秒
    @return true取出成功，false本次没有取出
    */
    bool take(char** packet, uint32_t milliseconds);

    /** 唯一消费者先按take规则取得首项，再取得当前已发布的FIFO前缀，不等待凑满
    @param [out] packets 至少maximum个指针位置，不能为nullptr；每项由调用者release
    @param [in] maximum 本次最多取得条数，范围1..64
    @param [out] consumed 已转移的所有权数量，未取出时为0
    @param [in] milliseconds 首项为空时最多等待毫秒
    @return true至少取得一项；停止或取消时调用者须归还整个已取得前缀
    */
    bool takePrefix(char** packets, size_t maximum, size_t& consumed, uint32_t milliseconds);

    /** 不等待地取得已提交正文，只由唯一消费者或静止清理方调用。
    @param [out] packet 正文所有权
    @return true成功
    */
    bool pop(char** packet);

    /** 合并提示，调用者先发布实际条件；不是许可计数。
    */
    void notify();

    /** 无条件唤醒，用于停止和排空；不要求当前有排队正文。
    */
    void wake();

    /** 静止期取消尚未取出的正文并清除通知。
    */
    void clear();

    /** 观察队列是否暂时没有排队项，不包含正在处理的正文。
    @return true当前没有排队项
    */
    bool empty() const;

    /** 观察在途计费总量，包含每条正文的队列及分配管理余量。
    @return 有限额度下已预留字节数；-1模式不计费并返回0，不能据此判断队列为空
    */
    uint64_t used() const;

    /** 观察是否仍有本对象分配且尚未释放的正文
    @return 包含未发布及已取出正文的弱一致存活快照，静止期用于配置保护
    */
    bool hasPending() const;

    /** 有限频率刷新系统内存压力，不等待另一刷新者。
    */
    void refreshMemory();

private:
    /** 禁止复制拥有映射或正文的对象
    @param [in] other 不可复制来源
    */
    ProcessWorkLocalQueue(const ProcessWorkLocalQueue& other);

    /** 禁止覆盖独占资源所有权
    @param [in] other 不可复制来源
    @return 不可调用
    */
    ProcessWorkLocalQueue& operator=(const ProcessWorkLocalQueue& other);

private:
    // 静止配置的小包备用池，不能取到槽时仍走原正文分配及错误分类
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    ProcessWorkPacketPool m_packetPool;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 拥有排队正文指针的MPSC队列，正文另行计费
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueue<char*> m_values;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 本地休眠唤醒后端，不表示队列元素数量
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    Semaphore m_notification;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 本轮数据通知是否已经发布
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<bool> m_notified;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 有限额度保存计费字节，-1模式只保存存活正文个数；配置只能在归零后修改
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<uint64_t> m_pending;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 下一次允许刷新系统压力的单调毫秒
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<uint64_t> m_nextMemoryCheck;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 最近一次系统内存余量检查结果
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::atomic<bool> m_memoryAvailable;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 静止配置后在运行期间只读的内存策略
    ProcessWorkMemoryConfig m_config;
    // 唯一消费者维护前缀取件次数，安排有限预算维护，不与其他消费者并发
    uint32_t m_prefixBatches;
};