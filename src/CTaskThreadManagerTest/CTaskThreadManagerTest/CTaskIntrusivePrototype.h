#pragma once
#include "LockFreeQueue/LockFreeQueueChannel.h"
#include <atomic>
#include <memory>
#include <stddef.h>
#include <stdint.h>

/** 只用于测试的永久存活节点；本原型不声称覆盖任务、额度和回收语义
*/
struct CTaskPrototypeNode
{
public:
    std::atomic<CTaskPrototypeNode*> m_next;
    uint64_t m_sequence;
    size_t m_producer;
    unsigned char m_padding[64 - sizeof(std::atomic<CTaskPrototypeNode*>) - sizeof(uint64_t) - sizeof(size_t)];

public:
    /** 初始化链字段及校验数据
    */
    CTaskPrototypeNode();

};

/** 帮助推进尾指针的 MPSC 原型；全部节点存活到所有线程退出
*/
class CTaskPrototypeIntrusive
{
public:
    /** 建立永久哨兵
    */
    CTaskPrototypeIntrusive();

    /** 发布独占且尚未入队的永久节点
    @param [in,out] node 完整初始化且 next 为空的节点
    */
    void push(CTaskPrototypeNode* node);

    /** 唯一消费者观察并推进；节点仍由测试持有
    @return 成功项或 nullptr
    */
    CTaskPrototypeNode* pop();

private:
    CTaskPrototypeNode m_dummy;
    CTaskPrototypeNode* m_head;
    unsigned char m_headPadding[64];
    std::atomic<CTaskPrototypeNode*> m_tail;
    unsigned char m_tailPadding[64];
};

/** 永久发布段集合；只使用固定数组，不在计时内申请内存或复用段
*/
class CTaskPrototypeSegments
{
public:
    /** 分配并初始化指定数量的段
    @param [in] count 初始总段数
    */
    explicit CTaskPrototypeSegments(size_t count);

    /** 领取尚未使用的永久段
    @return 可用段或 nullptr
    */
    LockFreeQueueChannelSegment<CTaskPrototypeNode>* acquire();

private:
    std::unique_ptr<LockFreeQueueChannelSegment<CTaskPrototypeNode>[]> m_segments;
    const size_t m_count;
    std::atomic<size_t> m_next;
};

/** 每个发布者独占一个候选缓存；发布竞争失败不浪费永久段预算
*/
class CTaskPrototypeSegmentLease
{
public:
    /** 绑定永久段集合
    @param [in,out] segments 生命周期覆盖本对象的集合
    */
    explicit CTaskPrototypeSegmentLease(CTaskPrototypeSegments& segments);

    /** 领取已清零段
    @return 候选段或 nullptr
    */
    LockFreeQueueChannelSegment<CTaskPrototypeNode>* acquire();

    /** 保存本发布者尚未发布的候选
    @param [in] segment 本调用独占的候选
    */
    void release(LockFreeQueueChannelSegment<CTaskPrototypeNode>* segment);

    /** 保持退役段存活，原型不计回收开销
    @param [in] segment 已摘除段
    */
    void retire(LockFreeQueueChannelSegment<CTaskPrototypeNode>* segment);

private:
    CTaskPrototypeSegments* m_segments;
    LockFreeQueueChannelSegment<CTaskPrototypeNode>* m_candidate;
};

/** 发布段保护登记，测试期间没有段释放
*/
struct CTaskPrototypePublisher
{
public:
    // 当前发布段的保护地址
    std::atomic<LockFreeQueueChannelSegment<CTaskPrototypeNode>*> m_hazard;
    // 当前发布段的唯一代次，用于使上一段的私有提示失效
    uint64_t m_generation;
    // 本发布者已经观察到的连续发布前缀
    size_t m_hint;
    // 原子发布竞争统计
    std::atomic<uint64_t> m_retries;
    // 与新版通道记录保持一致的调度让步状态。
    LockFreeQueueProgress m_progress;

public:
    /** 建立空登记
    */
    CTaskPrototypePublisher();

};

/** 使用当前生产代码的分段通道作为原型对照
*/
class CTaskPrototypeSegmented
{
public:
    /** 建立尚未绑定段的通道
    */
    CTaskPrototypeSegmented();

    /** 绑定初始哨兵段
    @param [in] first 尚未使用的永久段
    */
    void initialize(LockFreeQueueChannelSegment<CTaskPrototypeNode>* first);

    /** 发布永久节点
    @param [in,out] node 完整初始化的节点
    @param [in,out] publisher 独占的发布登记
    @param [in,out] lease 独占的段缓存
    @return 是否成功
    */
    bool push(CTaskPrototypeNode* node, CTaskPrototypePublisher& publisher, CTaskPrototypeSegmentLease& lease);

    /** 唯一消费者读取并推进
    @param [in,out] lease 消费者独占的段适配器
    @return 节点或 nullptr
    */
    CTaskPrototypeNode* pop(CTaskPrototypeSegmentLease& lease);

private:
    LockFreeQueueChannel<CTaskPrototypeNode, CTaskPrototypeSegmentLease, CTaskPrototypePublisher> m_channel;
};

/** 每生产者独占的结果；观察者仅在线程 join 后读取普通字段
*/
struct CTaskPrototypeProgress
{
public:
    uint64_t m_finished;
    uint64_t m_consumed;
    unsigned char m_padding[48];

public:
    /** 建立零结果
    */
    CTaskPrototypeProgress();

};

/** 独立运行原型对比，不计为 CTaskThreadManager 的验收吞吐
@param [in] argc 参数数量
@param [in] argv 模式、生产者数、任务数、级别数、CSV 路径
@return 0 表示校验通过
*/
int taskRunIntrusivePrototype(int argc, char* argv[]);