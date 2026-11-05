#pragma once

#include "LockFreeQueueChannelModels.h"
#include <stddef.h>
#include <stdint.h>
#include <algorithm>
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#endif

// SegmentPool contract:
// acquire(): exclusive, reset, non-null segment OR explicit allocation failure.
// release(candidate): return an unpublished, exclusively owned candidate.
// retire(segment): sole consumer; do not reset/free until all hazards exclude it.
// Pool operations must never wait for a particular paused participant.
template <class Node, class SegmentPool, class RecordType>
class LockFreeQueueChannel
{
public:
    typedef LockFreeQueueChannelSegment<Node> Segment;
    typedef RecordType Record;

    /** 建立本对象确定的初始状态；所属类型约定其所有权与生命周期
    @param [in] first 当前生命周期首段，nullptr 表示未初始化
    */
    explicit LockFreeQueueChannel(Segment* first);

    /** 重置独占或外部静止状态；不得与仍借用该对象的线程并发
    @param [in] first 当前生命周期首段，nullptr 表示未初始化
    */
    void reset(Segment* first);

    /** 将完整载荷以槽位 CAS 发布；成功后生产者不再解引用载荷
    @param [in,out] node 本次操作拥有或唯一消费者借用的非空节点
    @param [in,out] record 本次操作独占的登记槽；借用至返回
    @param [in,out] pool 发布段申请回收适配器；借用至返回
    @return 是否满足上述条件或完成上述操作
    */
    bool publish(Node* node, Record& record, SegmentPool& pool);

    /** 唯一消费者封闭观察为空的当前段；最多一个未消费封口，不生成空段链
    @param [in,out] pool 发布段申请回收适配器；借用至返回
    @return 是否满足上述条件或完成上述操作
    */
    bool seal(SegmentPool& pool);

    /** 取得当前首项的短期借用；最多跨一个段，不随积压遍历
    @param [in,out] pool 发布段申请回收适配器；借用至返回
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Node* front(SegmentPool& pool);

    /** 输出赋值成功后推进唯一消费者游标；仅在 front 返回有效节点时调用
    */
    void consume();

    /** 唯一消费者取得当前头段；借用至下一次段切换
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Segment* consumerSegment() const;

private:
    // CAS 失竞争后的有限退避指令数；不等待指定生产者
    static const size_t kContentionPauses = 2048;
    // 唯一消费者的当前段
    Segment* m_head;
    // 唯一消费者当前逻辑槽位
    size_t m_headIndex;
    // 唯一消费者尚未越过的部分封口，防止生成空段链
    Segment* m_pendingSeal;
    // 缓存行隔离填充，不承载业务状态
    char m_consumerPadding[64];
    // 发布尾段或已验证的前缀提示，均不预占载荷位置
    LockFreeQueueAtomic<Segment*> m_tail;
    // 缓存行隔离填充，不承载业务状态
    char m_tailPadding[64];

private:
    /** 越过一个已结束的段；冷路径不扩大常见首槽读取的内联代码
    @param [in,out] pool 发布段回收适配器
    @return 下一段首项或空指针
    */
    Node* advance(SegmentPool& pool);

    /** 保护并验证尾段；代次改变时重置私有位置提示
    @param [in,out] record 本次操作独占的登记槽；借用至返回
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Segment* protect(Record& record);

    /** 推进已证明全部发布的前缀提示；不预占未发布位置
    @param [in,out] segment 受保护或独占的发布段；借用至返回
    @param [in] position 已验证的逻辑槽位下标
    */
    static void help(Segment* segment, size_t position);

    /** 发布段结束标记；任何生产者均可帮助，不能等待原关闭者
    @param [in,out] segment 受保护或独占的发布段；借用至返回
    @param [in] position 已验证的逻辑槽位下标
    */
    static void close(Segment* segment, size_t position);

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueueChannel(const LockFreeQueueChannel& other) = delete;

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueueChannel& operator=(const LockFreeQueueChannel& other) = delete;
};

#include "LockFreeQueueChannel.inl"