#pragma once
#include "ProcessWorkMacro.h"
#include <atomic>
#include <memory>
#include <stdint.h>

class ProcessWorkChannel;
class ProcessWorkSendCache;

/** 不可变的发送通道缓存项；摘除后等待所有借用结束再释放拥有引用
*/
struct ProcessWorkSendCacheNode
{
public:
    // 本节点对应的目标进程
    int32_t m_pid;
    // 发布前观察到的缓存清理代次
    uint64_t m_epoch;
    // 独立拥有通道，缓存借用期间不复制引用计数
    std::shared_ptr<ProcessWorkChannel> m_channel;
    // 仅退役链所有者修改，不参与读取通道
    ProcessWorkSendCacheNode* m_next;

public:
    /** 保存目标及拥有引用，构造失败由缓存入口捕获
    @param [in] pid 目标进程
    @param [in] epoch 清理代次
    @param [in] channel 已完整准备的通道
    */
    ProcessWorkSendCacheNode(int32_t pid, uint64_t epoch, const std::shared_ptr<ProcessWorkChannel>& channel);
};

/** 有界的线程保护记录；登记后不转给另一线程，嵌套借用回退到原有Map
*/
struct ProcessWorkSendCacheRecord
{
public:
    // 唯一登记号，0未登记；避免系统线程ID复用混淆旧记录
    std::atomic<uint64_t> m_thread;
    // 该线程当前借用的节点；发布后验证来源，清理扫描它决定回收
    std::atomic<ProcessWorkSendCacheNode*> m_pointer;
    // 分离各线程经常写入的保护位置
    char m_padding[48];

public:
    /** 建立未登记且无借用的记录
    */
    ProcessWorkSendCacheRecord();
};

/** 一次发送的借用凭据，不阻塞清缓存，也不复制shared_ptr
*/
class ProcessWorkSendCacheLease
{
public:
    /** 建立空借用
    */
    ProcessWorkAPI ProcessWorkSendCacheLease();

    /** 结束借用并尝试回收已摘除节点
    */
    ProcessWorkAPI ~ProcessWorkSendCacheLease();

    /** 提前结束当前借用，之后可以再次find；空借用无操作
    */
    ProcessWorkAPI void reset();

public:
    // 借用的缓存域，生命周期覆盖本次发送
    ProcessWorkSendCache* m_cache;
    // 本线程独占使用的保护记录，空表示未取得
    ProcessWorkSendCacheRecord* m_record;

private:
    /** 禁止复制借用所有权
    @param [in] other 不可复制来源
    */
    ProcessWorkSendCacheLease(const ProcessWorkSendCacheLease& other);

    /** 禁止覆盖借用所有权
    @param [in] other 不可复制来源
    @return 不可调用
    */
    ProcessWorkSendCacheLease& operator=(const ProcessWorkSendCacheLease& other);
};

/** PID发送的有界快缓存；原Map仍是查找与创建的权威入口。
    每个槽只填入首次目标，碰撞直接回退而不反复换项；记录耗尽、重入或分配失败也不影响发送。
    clear与发送可并发；析构要求外部发送已停止，不等待暂停的借用者。
*/
class ProcessWorkSendCache
{
public:
    /** 建立固定大小的空缓存和保护记录，不申请通道
    */
    ProcessWorkAPI ProcessWorkSendCache();

    /** 静止销毁全部缓存及退役节点
    */
    ProcessWorkAPI ~ProcessWorkSendCache();

    /** 借用匹配目标的通道，一次保护冲突即回退，不在本层无限自旋
    @param [in] pid 目标进程
    @param [in,out] lease 本次调用的空借用凭据
    @return 通道借用或nullptr，有效期到lease销毁
    */
    ProcessWorkAPI ProcessWorkChannel* find(int32_t pid, ProcessWorkSendCacheLease& lease);

    /** 观察本次权威查找开始前的清理代次
    @return 非零代次，0表示计数耗尽后停用缓存
    */
    ProcessWorkAPI uint64_t epoch() const;

    /** 缓存一次权威查找结果，不影响原发送的成功与否
    @param [in] pid 目标进程
    @param [in] expectedEpoch 权威查找开始前的代次
    @param [in] channel 完整通道拥有引用
    */
    ProcessWorkAPI void remember(int32_t pid, uint64_t expectedEpoch, const std::shared_ptr<ProcessWorkChannel>& channel);

    /** 使全部快缓存失效，立即摘除节点，仍在发送的借用可完成
    */
    ProcessWorkAPI void clear();

    /** 非等待地回收当前未受保护的退役节点，可由多个线程调用
    */
    ProcessWorkAPI void collect();

private:
    /** 为当前线程取得稳定记录，耗尽和嵌套借用返回空
    @return 独占使用的记录或nullptr
    */
    ProcessWorkSendCacheRecord* record();

    /** 发布一个已摘除节点到待回收链
    @param [in] node 已无缓存槽引用的独占节点，允许空
    */
    void retire(ProcessWorkSendCacheNode* node);

    /** 禁止复制资源及保护域
    @param [in] other 不可复制来源
    */
    ProcessWorkSendCache(const ProcessWorkSendCache& other);

    /** 禁止覆盖资源及保护域
    @param [in] other 不可复制来源
    @return 不可调用
    */
    ProcessWorkSendCache& operator=(const ProcessWorkSendCache& other);

private:
    // 实例身份，避免TLS提示命中地址复用后的另一个缓存域
    uint64_t m_identity;
    // 清理代次，耗尽后永久停用快缓存
    std::atomic<uint64_t> m_epoch;
    // PID取模的有限缓存槽；碰撞只影响命中率
    std::atomic<ProcessWorkSendCacheNode*> m_slots[64];
    // 固定128个登记记录，耗尽后回退原Map，不动态扩容
    ProcessWorkSendCacheRecord m_records[128];
    // 已摘除但可能仍在发送的节点
    std::atomic<ProcessWorkSendCacheNode*> m_retired;
};