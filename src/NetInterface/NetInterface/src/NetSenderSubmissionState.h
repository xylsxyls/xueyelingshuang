#pragma once
#include "NetSenderSubmission.h"

class NetSenderSubmissionEntry;

// 请求的唯一接纳阶段；流帧只使用QUEUED和RETIRED。
enum NetSenderSubmissionPhase
{
    // 只预留身份，未接纳正文，不参与FIFO。
    NET_SUBMISSION_RESERVED,
    // 已拥有正文，等待登记协议请求或发送流帧。
    NET_SUBMISSION_QUEUED,
    // 请求已登记协议表，正文仍归提交层所有。
    NET_SUBMISSION_REGISTERED,
    // 请求正文已由协议接管，仍等待唯一业务终态。
    NET_SUBMISSION_SENT,
    // 已从可见集合移除；只可能因owner借用而延迟释放。
    NET_SUBMISSION_RETIRED
};

// 同步交付回执独立于业务终态，由原等待者读完后归还。
struct NetSenderSubmissionReceipt
{
public:
    // 所属表的下一个回执；最多128个。
    NetSenderSubmissionReceipt* m_next;
    // 正文尚存时的非拥有关联，正文释放前清空。
    NetSenderSubmissionEntry* m_entry;
    // 本实例单调递增且不复用的票号。
    uint64_t m_ticket;
    // 归属连接代次，禁止由新连接结算旧回执。
    uint64_t m_generation;
    // 包含排队时间的原始单调截止。
    uint64_t m_deadline;
    // READY未完成；OK交付；负数为确定失败。
    int32_t m_result;
    // 协议已接纳，等待整个下游链交付。
    bool m_staged;

public:
    /** 创建未关联、未完成的回执
    */
    NetSenderSubmissionReceipt();
};

// 一次正文只有一个拥有者；嵌入式链结不另分配队列节点或共享引用计数。
class NetSenderSubmissionEntry
{
public:
    /** 创建空预留；所有权随后交给接纳状态
    */
    NetSenderSubmissionEntry();

    /** 擦除并释放仍拥有的正文，预算由状态在此前归还
    */
    ~NetSenderSubmissionEntry();

    /** 原子复制正文；失败不改变已有输入
    @param [in] bytes 已验证地址范围，size非零时非空
    @param [in] size 已验证硬上限，单位字节
    @return 是否成功取得独占副本
    */
    bool copy(const uint8_t* bytes, uint32_t size);

    /** 擦除并释放正文，不处理外层预算
    */
    void clear();

public:
    // 固定请求身份或流帧元数据；借用输出在next时填写。
    NetSenderSubmissionCommand m_command;
    // 本对象拥有的正文，任何回调都不能提前释放owner正在借用的数据。
    uint8_t* m_input;
    // 当前实际拥有的字节数。
    uint32_t m_size;
    // 唯一阶段，不组合已接纳／已登记／已发送三个独立标记。
    NetSenderSubmissionPhase m_phase;
    // 请求表中的前一项，流帧不登记请求表。
    NetSenderSubmissionEntry* m_previousRequest;
    // 请求表中的后一项。
    NetSenderSubmissionEntry* m_nextRequest;
    // 混合请求／流帧FIFO中的前一项。
    NetSenderSubmissionEntry* m_previousPending;
    // 同一FIFO中的后一项。
    NetSenderSubmissionEntry* m_nextPending;
    // 可空的非拥有回执关联，回执消费前清空。
    NetSenderSubmissionReceipt* m_receipt;
    // 当前是否仍在待发送FIFO中。
    bool m_pending;
    // 请求取消意图，不依赖数据队列空位。
    bool m_cancelled;
    // 取消或本地终态是否已经交给owner。
    bool m_cancelTaken;
    // 同轮背压最多尝试一次；后面的业务不能越过队首。
    uint64_t m_attempt;

private:
    /** 禁止复制正文和链表归属
    @param [in] other 原对象
    */
    NetSenderSubmissionEntry(const NetSenderSubmissionEntry& other);

    /** 禁止复制所有权
    @param [in] other 原对象
    @return 不可调用
    */
    NetSenderSubmissionEntry& operator=(const NetSenderSubmissionEntry& other);
};

// 外层可选短锁的作用域；单执行者Peer使用空门，不创建线程。
class NetSenderSubmissionGuard
{
public:
    /** 进入外层提供的短临界区
    @param [in] gate 成对且不抛异常的函数表，context覆盖本次调用
    */
    explicit NetSenderSubmissionGuard(const NetSenderSubmissionGate& gate);

    /** 离开同一次临界区
    */
    ~NetSenderSubmissionGuard();

private:
    /** 禁止复制锁持有权
    @param [in] other 原作用域
    */
    NetSenderSubmissionGuard(const NetSenderSubmissionGuard& other);

    /** 禁止复制锁持有权
    @param [in] other 原作用域
    @return 不可调用
    */
    NetSenderSubmissionGuard& operator=(const NetSenderSubmissionGuard& other);

private:
    // 本次取得的函数表副本。
    NetSenderSubmissionGate m_gate;
};

// 提交门内唯一权威状态；请求表负责终态，FIFO只负责实际正文提交顺序。
class NetSenderSubmissionState
{
public:
    /** 初始化无输入、无回执的静止状态
    */
    NetSenderSubmissionState();

    /** 所有执行者退出后释放剩余所有权，不调用外部函数
    */
    ~NetSenderSubmissionState();

    /** 查找未终态请求，不返回已退休的借用项
    @param [in] id 本实例原号
    @return 借用项或nullptr，最多遍历128项
    */
    NetSenderSubmissionEntry* find(uint64_t id) const;

    /** 将成功接纳的正文接到唯一FIFO尾部，不分配内存
    @param [in] entry 尚未进入FIFO的本实例项
    */
    void append(NetSenderSubmissionEntry* entry);

    /** 从FIFO摘除任意项，不改变正文借用或请求终态
    @param [in] entry 本实例项，重复摘除安全
    */
    void remove(NetSenderSubmissionEntry* entry);

    /** 擦除正文并归还对应数据／控制字节预算
    @param [in] entry 仍拥有输入的本实例项
    */
    void clear(NetSenderSubmissionEntry* entry);

    /** 摘除待发项并释放；当前借用项只标记退休，finish负责最终释放
    @param [in] entry 已经退出请求表的本实例项
    */
    void retire(NetSenderSubmissionEntry* entry);

    /** 取走请求的唯一终态身份，保持在途正文借用有效
    @param [in] entry 尚在请求表中的请求
    @param [out] command 可信原始身份
    */
    void take(NetSenderSubmissionEntry* entry, NetSenderSubmissionCommand& command);

    /** 创建尚未公开的回执，失败不占预算
    @param [in] generation 所属代次
    @param [in] deadline 原始单调截止
    @return 独占待登记回执或nullptr
    */
    NetSenderSubmissionReceipt* createReceipt(uint64_t generation, uint64_t deadline);

    /** 同时提交回执与正文关联，不分配内存
    @param [in] receipt 已创建且未登记的回执
    @param [in] entry 已成功接纳的正文
    */
    void addReceipt(NetSenderSubmissionReceipt* receipt, NetSenderSubmissionEntry* entry);

    /** 无借用动作时移除已过期且尚未交协议的同步流帧
    @param [in] nowMs 单调毫秒，不延长原期限
    */
    void expireFrames(uint64_t nowMs);

    /** 结算一个回执并清除对应的排空栅栏，终态不可覆盖
    @param [in] receipt 本表回执
    @param [in] result 最终结果，不能是READY
    */
    void settle(NetSenderSubmissionReceipt* receipt, int32_t result);

    /** 清理普通及控制流输入，保留owner当前借用
    */
    void dropFrames();

    /** 观察是否仍需本代运输推进；已完成但未读回执不会阻止自动恢复
    @return 是否还有READY回执，调用者已持提交门
    */
    bool pendingReceipts() const;

public:
    // 每实例最多128个业务预留。
    static const uint32_t kRequests = 128;
    // 请求和普通流共享2MiB输入预算。
    static const uint32_t kInputBytes = 2097152;
    // 普通流最多64项。
    static const uint32_t kFrames = 64;
    // 独立控制输入最多128KiB。
    static const uint32_t kControlBytes = 131072;
    // 控制输入最多16项。
    static const uint32_t kControls = 16;
    // 借用的成对锁及锁外唤醒，生命周期内只配置一次。
    NetSenderSubmissionGate m_gate;
    // 是否已经完成门配置。
    bool m_gateConfigured;
    // 当前连接代次。
    uint64_t m_generation;
    // 固定产品，0允许旧通用连接选择产品。
    uint32_t m_product;
    // 请求号不跨代次复用。
    uint64_t m_nextId;
    // 流帧动作号不跨代次复用。
    uint64_t m_nextFrame;
    // 同步回执号不跨代次复用。
    uint64_t m_nextSync;
    // 本轮owner推进号，不影响接纳顺序。
    uint64_t m_round;
    // 握手协商的请求上限。
    uint32_t m_limit;
    // 握手协商的流块上限。
    uint32_t m_chunk;
    // 已拥有的请求和普通流字节数，包含当前借用。
    uint32_t m_bytes;
    // 已拥有的控制字节数，包含当前借用。
    uint32_t m_controlBytes;
    // 尚无业务终态的请求数。
    uint32_t m_requestCount;
    // 尚未释放的普通流项数。
    uint32_t m_frameCount;
    // 尚未释放的控制项数。
    uint32_t m_controlCount;
    // 尚未被等待者消费的回执数，完成项也占预算。
    uint32_t m_receiptCount;
    // 接纳快照，只在握手完成且未关闭时为true。
    bool m_ready;
    // 无分配的关闭意图，第一次原因保留。
    bool m_closing;
    // 关闭是否已经交给owner。
    bool m_closeTaken;
    // 第一次关闭原因。
    int32_t m_reason;
    // 按预留次序登记的请求首项，拥有到终态或退休借用结束。
    NetSenderSubmissionEntry* m_requests;
    // 请求表尾项，仅用于常数时间追加。
    NetSenderSubmissionEntry* m_requestsTail;
    // 所有已接纳正文的统一FIFO，控制只有独立容量而不能越过正文。
    NetSenderSubmissionEntry* m_pending;
    // 统一FIFO尾项。
    NetSenderSubmissionEntry* m_pendingTail;
    // owner当前借用，最多一项，不交给生产者释放。
    NetSenderSubmissionEntry* m_taken;
    // owner当前动作，CLOSE没有关联正文。
    int32_t m_takenKind;
    // 本表拥有的回执链，最多128项。
    NetSenderSubmissionReceipt* m_receipts;
    // 当前已交协议但尚未全链路交付的回执，非拥有。
    NetSenderSubmissionReceipt* m_draining;
};