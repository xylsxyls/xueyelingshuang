#pragma once

#include "LockFreeQueueChannelModels.h"
#include "LockFreeQueueProgress.h"
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
    @param [in] cancelableReservations 是否使用可取消预约顺序，默认保持发布 CAS 顺序
    */
    void reset(Segment* first, bool cancelableReservations = false);

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

    /** 取得当前首项的短期借用；默认方式最多跨一个段，预约方式可越过冻结范围内的空洞
    @param [in,out] pool 发布段申请回收适配器；借用至返回
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Node* front(SegmentPool& pool);

    /** 唯一消费者只读探测当前首槽，不推进段或消费游标
    @return false 表示观察到首槽为空或尚未初始化；true 表示可能就绪，需要正常 front 确认
    */
    bool possiblyReady() const;

    /** 观察当前发布段中连续就绪的前缀，不移动消费位置
    @param [out] nodes 至少 maximum 个指针的输出数组
    @param [in] maximum 本次最多观察数量
    @return 就绪前缀长度；仅唯一消费者调用，调用前 front 已处理段边界
    */
    size_t readyPrefix(Node** nodes, size_t maximum) const;

    /** 输出赋值成功后推进唯一消费者游标；仅消费当前段已经观察到的就绪前缀
    @param [in] count 已经成功赋值的连续项数，不越过当前段边界；零表示未消费
    */
    void consume(size_t count = 1);

    /** 唯一消费者取得当前头段；借用至下一次段切换
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Segment* consumerSegment() const;

private:
    // 初始化后冻结；两种发布算法不在同一生命周期中混用
    bool m_cancelableReservations;
    // 每次投递读取的冻结模式与消费者游标隔离，避免只读配置被游标写入反复失效
    char m_configurationPadding[64];
    // 唯一消费者的当前段
    Segment* m_head;
    // 唯一消费者当前逻辑槽位
    size_t m_headIndex;
    // 当前槽的稳定地址；段末及未初始化时为空，避免空检查重复计算映射下标
    LockFreeQueueAtomic<uintptr_t>* m_frontCell;
    // 唯一消费者尚未越过的部分封口，防止生成空段链
    Segment* m_pendingSeal;
    // 唯一消费者的连续操作计数，空轮询和批量消费均计入调度让步。
    LockFreeQueueProgress m_consumerProgress;
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
    void close(Segment* segment, size_t position);

    /** 预约方式发布完整载荷，预约被取消则重新选择位置；不转交失败载荷
    @param [in,out] node 本调用独占的完整载荷
    @param [in,out] record 本调用独占并持续保护发布段的登记
    @param [in,out] pool 发布段申请和归还接口
    @return 是否成功提交载荷；失败发生在所有权转交前
    */
    bool publishReserved(Node* node, Record& record, SegmentPool& pool);

    /** 唯一消费者跳过可取消预约；首次空观察冻结范围，不追赶后来的无限发布
    @param [in,out] pool 发布段退休接口
    @return 已发布载荷，或在本次观察范围内为空
    */
    Node* frontReserved(SegmentPool& pool);

    /** 唯一消费者检查冻结范围内是否有已提交载荷，避免取消所有仅在途预约
    @param [in] boundary 仍由消费者可达链保持存活的末段
    @param [in] boundaryIndex 末段内观察上界，不超过段容量
    @return 范围内是否存在载荷；不消费，不反转节点
    */
    bool publishedBefore(Segment* boundary, size_t boundaryIndex);

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