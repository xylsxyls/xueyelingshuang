#pragma once
#include "NetSenderSubmission.h"
#include "NetObjectPool.h"
#include "NetBufferPool.h"
#include "LockFreeHashMap/LockFreeHashMapAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <atomic>
#include <memory>

class NetSenderSubmissionEntry;

// 生产者查找的独立请求身份；协议链表和正文永远不由生产者访问
struct NetSenderSubmissionFront
{
public:
    /** 创建尚未发布的身份
    */
    NetSenderSubmissionFront();
public:
    // 仅owner使用的关联记录；入队前设置，退休后不得解引用
    NetSenderSubmissionEntry* m_entry;
    // 发布后不变的可信请求元数据
    NetSenderSubmissionCommand m_command;
    // BEGIN入口已提交且可被其他生产者使用
    std::atomic<bool> m_published;
    // 取消无需队列槽位，也不访问owner链表
    std::atomic<bool> m_cancelled;
    // owner已取得唯一业务终态，不再允许新正文
    std::atomic<bool> m_retired;
    // 0未send，1正在准备，2已提交；只对同一请求拒绝重复send，不作为全局锁
    std::atomic<uint32_t> m_sent;
};

// 正文及预算一起转移；最后引用归还才擦除，owner借用期间不能提前释放
class NetSenderSubmissionPayload
{
public:
    /** 先取得实际池槽再复制；分配失败也通过status报告
    @param [in,out] budget 生命周期覆盖所有入口及owner借用的原子字节计数
    @param [in,out] pool 实际正文池，生命期覆盖本对象
    @param [in] bytes 已验证的借用数据
    @param [in] size 长度
    */
    NetSenderSubmissionPayload(NetBufferPool& pool, std::atomic<uint64_t>& budget, const uint8_t* bytes, uint32_t size);

    /** 擦除正文并精确归还一次已取得的额度
    */
    ~NetSenderSubmissionPayload();
public:
    // 被转移的独占正文，0字节为空
    uint8_t* m_bytes;
    // 正文大小
    uint32_t m_size;
    // 构造结果，只有OK表示已取得预算和正文
    int32_t m_status;
private:
    /** 禁止复制正文所有权
    @param [in] other 原对象
    */
    NetSenderSubmissionPayload(const NetSenderSubmissionPayload& other);

    /** 禁止重复归还预算
    @param [in] other 原对象
    @return 不可调用
    */
    NetSenderSubmissionPayload& operator=(const NetSenderSubmissionPayload& other);
private:
    // 所属状态的预算，状态必须在所有调用及借用结束后销毁
    std::atomic<uint64_t>& m_budget;
    // 独占物理缓冲，最后引用析构时归还
    NetBufferLease m_buffer;
    // 仅成功预留时归还
    bool m_reserved;
};

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
    // 所属表的下一个回执；由实际回执池控制。
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
    std::atomic<int32_t> m_result;
    // 原等待者已读完，owner在安全点摘除索引和链结
    std::atomic<bool> m_consumed;
    // 位1登记占用、位2引用已发布、位4结果已完成；共同RMW保证登记/完成不互相漏看
    std::atomic<uint32_t> m_signalState;
    // 状态2后不可修改；票据持有引用覆盖执行者notify的完整调用
    std::shared_ptr<NetSenderSubmissionSignal> m_signal;
    // 协议已接纳，等待整个下游链交付。
    bool m_staged;

public:
    /** 创建未关联、未完成的回执
    */
    NetSenderSubmissionReceipt();
};

// 协议链结由owner独占；正文引用覆盖生产者返回及owner借用。
class NetSenderSubmissionEntry
{
public:
    /** 创建空预留；所有权随后交给接纳状态
    */
    NetSenderSubmissionEntry();

    /** 释放正文引用，最后引用负责擦除及归还预算
    */
    ~NetSenderSubmissionEntry();

    /** 解除正文借用并释放引用，不提前归还其他引用仍占用的预算
    */
    void clear();

public:
    // 池记录为物理槽地址；m_pool为空的内联工作区指向持有正文的入口项
    void* m_storage;
    // 所属记录池，覆盖本记录生命期；内联工作区为空，由owner按批复用
    NetObjectPool<NetSenderSubmissionEntry>* m_pool;
    // 可空的生产者身份，正文和链结不放入此共享对象
    NetObjectReference<NetSenderSubmissionFront> m_front;
    // 正文引用覆盖next/finish借用，与入口发布者同时持有也可安全回收
    NetObjectReference<NetSenderSubmissionPayload> m_payload;
    // 普通异步DATA直接拥有正文；大于64字节时使用现有正文池
    NetBufferLease m_frameBuffer;
    // 已登记的异步DATA字节统计；clear后置空，避免重复归还
    std::atomic<uint64_t>* m_frameBudget;
    // 固定请求身份或流帧元数据；内联DATA的帧借用m_storage入口，next时才汇总到命令。
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

// 多生产者入口项；BEGIN/DATA在接纳前备齐owner元数据，不在已接纳后补分配
struct NetSenderSubmissionInput
{
public:
    /** 统计已由物理池接纳的入口项，不另设条数上限
    @param [in,out] count 生命周期覆盖本入口项的原子计数
    */
    explicit NetSenderSubmissionInput(std::atomic<uint32_t>& count);

    /** 最后一个生产者或owner引用退出后归还入口统计
    */
    ~NetSenderSubmissionInput();
public:
    // BEGIN登记身份，SEND交正文，DATA交流帧
    int32_t m_kind;
    // BEGIN或DATA预分配的owner对象，导入时release转给owner链表
    NetObjectReference<NetSenderSubmissionEntry> m_entry;
    // SEND携带可信身份，不从裸owner指针查找
    NetObjectReference<NetSenderSubmissionFront> m_front;
    // SEND/DATA拥有输入引用
    NetObjectReference<NetSenderSubmissionPayload> m_payload;
    // 可空同步回执；索引保留至等待者消费和owner摘除
    NetObjectReference<NetSenderSubmissionReceipt> m_receipt;
private:
    // 比本对象活得更久的预算，最后引用释放时仅归还一次
    std::atomic<uint32_t>& m_count;
};

// 统一入口中的两种所有权：RPC/同步控制保留输入记录，普通DATA直接携带帧记录
struct NetSenderSubmissionItem
{
public:
    // 短DATA直接占入口节点；元数据及常见短正文连续，减少生产和消费间的缓存行传递
    NetInterfaceFrame m_inlineFrame;
    // 短DATA所属代次，入队前复制为只读值
    uint64_t m_inlineGeneration;
    // 不超过64字节的实际正文；复制入队并在每个拥有副本销毁时擦除
    uint8_t m_inlineBody[64];
    // 仅栈上提交描述借用调用方正文；队列构造/消费复制后恒为false，不发布外部指针
    bool m_borrowedInline;
    // RPC或控制输入；与m_frame最多一项有效，inlineFrame.kind为0时使用
    NetObjectReference<NetSenderSubmissionInput> m_input;
    // 异步长DATA的独占转交地址：发布前由生产者拥有，成功后只由owner链表释放；失败由生产者回收
    NetSenderSubmissionEntry* m_frame;

public:
    /** 创建空队列输出项
    */
    NetSenderSubmissionItem();

    /** 创建已验证的短DATA提交描述；基础队列在返回前复制正文到自己的节点
    @param [in] frame 调用方已校验的DATA，正文不超过64字节；仅本次调用借用
    @param [in] generation 本次提交借用保护内取得的连接代次
    */
    NetSenderSubmissionItem(const NetInterfaceFrame& frame, uint64_t generation);

    /** 复制输入引用供原有RPC/控制路径使用
    @param [in] input 已完成构造的输入，复制所有权
    */
    NetSenderSubmissionItem(const NetObjectReference<NetSenderSubmissionInput>& input);

    /** 复制队列载荷；内联正文拥有独立存储，裸帧地址只转交一次给owner
    @param [in] other 已构造且在本次调用期间稳定的输入
    */
    NetSenderSubmissionItem(const NetSenderSubmissionItem& other);

    /** 替换消费缓冲；先擦除原内联正文，不释放已转交的裸帧地址
    @param [in] other 稳定的输入，允许自赋值
    @return 本对象
    */
    NetSenderSubmissionItem& operator=(const NetSenderSubmissionItem& other);

    /** 擦除本副本的短正文；裸帧生命周期由显式提交和owner回收控制
    */
    ~NetSenderSubmissionItem();
};

// 非独占提交借用；多个生产者同时进入，关闭代次只等待先进入的发布者
class NetSenderSubmissionAccess
{
public:
    /** 取得当前开放代次的非独占借用
    @param [in,out] state 生命周期覆盖本调用
    */
    explicit NetSenderSubmissionAccess(NetSenderSubmissionState& state);

    /** 完成队列发布及通知后归还
    */
    ~NetSenderSubmissionAccess();

    /** 查询本次是否进入开放代次
    @return 接纳资格，不等于业务接纳结果
    */
    bool accepted() const;
private:
    /** 禁止复制借用
    @param [in] other 原对象
    */
    NetSenderSubmissionAccess(const NetSenderSubmissionAccess& other);

    /** 禁止重复归还
    @param [in] other 原对象
    @return 不可调用
    */
    NetSenderSubmissionAccess& operator=(const NetSenderSubmissionAccess& other);
private:
    // 外层拥有的接纳状态
    NetSenderSubmissionState& m_state;
    // 成功时才归还计数
    bool m_accepted;
};

// 生产者只访问原子预算、身份索引和入口队列；其余链表由协议owner独占。
class NetSenderSubmissionState
{
public:
    /** 初始化无输入、无回执的静止状态
    @param [in] pools 各物理池配置，构造时复制
    */
    explicit NetSenderSubmissionState(const NetSenderSubmissionPools& pools = NetSenderSubmissionPools());

    /** 所有执行者退出后释放剩余所有权，不调用外部函数
    */
    ~NetSenderSubmissionState();

    /** owner关闭接纳并等已进入的发布者退出；不得从提交借用内调用
    */
    void quiesce();

    /** owner有界导入队列及清理已消费回执，不执行业务回调
    @param [in] deferData true允许保留纯短DATA前缀，供drainData直接借用
    */
    void pump(bool deferData = false);

    /** 将未消费的短DATA前缀恢复为通用FIFO；回调借用期间不改变存储
    */
    void materializeDirect();

    /** 丢弃剩余直接前缀；回调中清理时保留当前借用，只由owner调用
    */
    void dropDirect();

    /** 分配不回绕的本实例身份
    @param [in,out] counter 单调高水位
    @return 新非零编号，耗尽为0
    */
    static uint64_t nextId(std::atomic<uint64_t>& counter);

    /** 竞争时周期性让出时间片；不持有资源等待，也不把竞争作为提交失败
    @param [in,out] attempts 本次CAS循环的失败次数
    */
    static void pauseContention(uint32_t& attempts);

    /** 转换身份索引结果，容量不足与系统分配失败分别报告
    @param [in] result 索引操作结果，已提交时优先返回OK
    @return NI结果；调用方仍负责未提交时的所有权回滚
    */
    static int32_t indexResult(const LockFreeHashMapResult& result);

    /** 转换队列提交结果；满容量只返回背压，不在owner线程等待
    @param [in] result 使用重试竞争、不等待容量的队列操作结果
    @return NI结果，只有OK表示已移交队列
    */
    static int32_t queueResult(LockFreeQueueResult result);

    /** 无分配记录第一关闭原因；并发生产者可调用
    @param [in] reason NI原因，包括0
    */
    void requestClose(int32_t reason);

    /** 提交后提示owner，函数表在启动后不变
    */
    void wake();

    /** 创建并登记一个同步回执，失败不泄漏额度
    @param [in] generation 所属代次
    @param [in] deadline 原始期限
    @param [out] result OK、LIMIT_EXCEEDED或NO_MEMORY
    @return 本模块强引用，失败为空
    */
    NetObjectReference<NetSenderSubmissionReceipt> reserveReceipt(uint64_t generation, uint64_t deadline, int32_t& result);

    /** 发布失败时撤销尚未交给owner的票据
    @param [in] receipt 未发布的回执，可空
    */
    void rollbackReceipt(const NetObjectReference<NetSenderSubmissionReceipt>& receipt);

    /** 查找未终态请求，不返回已退休的借用项
    @param [in] id 本实例原号
    @return 借用项或nullptr，通过并发身份索引查找
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
    @return 是否还有READY回执，仅owner查询链表
    */
    bool pendingReceipts() const;

public:
    // 借用的owner通知/回收等待函数，生命周期内只配置一次。
    NetSenderSubmissionGate m_gate;
    // 是否已经完成通知配置。
    bool m_gateConfigured;
    // 当前连接代次。
    std::atomic<uint64_t> m_generation;
    // 固定产品，0允许旧通用连接选择产品。
    std::atomic<uint32_t> m_product;
    // 请求号不跨代次复用。
    std::atomic<uint64_t> m_nextId;
    // 同步回执号不跨代次复用。
    std::atomic<uint64_t> m_nextSync;
    // 本轮owner推进号，不影响接纳顺序。
    uint64_t m_round;
    // 同一调度轮内只做一次队列/索引域维护；短帧分批导入不重复扫描保护记录
    uint64_t m_maintainedRound;
    // 握手协商的流块上限。
    std::atomic<uint32_t> m_chunk;
    // 已拥有的请求和普通流字节数，包含当前借用。
    std::atomic<uint64_t> m_bytes;
    // 隔开并发接纳计数和owner完成计数；128字节还避开相邻缓存行预取，不要求特殊对齐分配
    uint8_t m_asyncBefore[128];
    // 异步DATA累计预留字节；失败回滚，成功只增；无符号差值表示实际借用
    std::atomic<uint64_t> m_asyncBytes;
    // owner归还侧与生产者侧之间的物理隔离
    uint8_t m_asyncBetween[128];
    // owner单写、外部只读的累计归还字节，关闭借用期间尚不归还
    std::atomic<uint64_t> m_asyncReturnedBytes;
    // 隔开后续生产者计数和生命周期快照
    uint8_t m_asyncAfter[128];
    // 已拥有的控制字节数，包含当前借用。
    std::atomic<uint64_t> m_controlBytes;
    // 尚无业务终态的请求数。
    std::atomic<uint32_t> m_requestCount;
    // 尚未释放的普通流项数。
    std::atomic<uint32_t> m_frameCount;
    // 尚未释放的控制项数。
    std::atomic<uint32_t> m_controlCount;
    // 入口项数，包含准备、发布及owner局部引用；即使零字节或原请求已终态也受限
    std::atomic<uint32_t> m_inputCount;
    // 尚未被等待者消费的回执数，完成项也占预算。
    std::atomic<uint32_t> m_receiptCount;
    // 尚未结算的回执数，包括仍在生产者入口的项
    std::atomic<uint32_t> m_pendingReceiptCount;
    // 接纳快照，只在握手完成且未关闭时为true。
    std::atomic<bool> m_ready;
    // 高32位非零表示显式关闭，低32位保存首次原因；生产者只更新此原子字段
    std::atomic<uint64_t> m_closeRequest;
    // 隔离生产者逐条更新的借用计数，owner读取ready/关闭快照不争用同一缓存行
    uint8_t m_admissionBefore[128];
    // 高位关闭、低31位为在途提交者；ready/start只在封闭且归零后修改代次配置
    std::atomic<uint32_t> m_admission;
    // 池对象的owner元数据不与在途生产者计数共享缓存行
    uint8_t m_admissionAfter[128];
    // 所有实际池和索引初始化的首个错误，构造后不可变
    int32_t m_initializationResult;
    // 正文池先于所有引用声明，保证最后析构
    NetBufferPool m_bodies;
    // 控制正文独立物理池
    NetBufferPool m_controlBodies;
    // 请求身份和生命周期记录，生命期覆盖以下索引和队列
    NetObjectPool<NetSenderSubmissionEntry> m_requestsPool;
    // 普通单向流记录，与未完成RPC隔离，生命期覆盖以下索引和队列
    NetObjectPool<NetSenderSubmissionEntry> m_framesPool;
    // ACK和取消记录，与普通数据隔离，生命期覆盖以下索引和队列
    NetObjectPool<NetSenderSubmissionEntry> m_controlsPool;
    // 并发请求身份，生命期覆盖以下索引和队列
    NetObjectPool<NetSenderSubmissionFront> m_frontsPool;
    // 同步交付回执，与业务应答隔离，生命期覆盖以下索引和队列
    NetObjectPool<NetSenderSubmissionReceipt> m_receiptsPool;
    // 生产者提交记录，出队后的局部借用仍占用池槽，生命期覆盖以下索引和队列
    NetObjectPool<NetSenderSubmissionInput> m_inputsPool;
    // 普通正文所有权记录，生命期覆盖以下索引和队列
    NetObjectPool<NetSenderSubmissionPayload> m_payloadsPool;
    // 控制正文所有权记录，生命期覆盖以下索引和队列
    NetObjectPool<NetSenderSubmissionPayload> m_controlPayloadsPool;
    // 身份索引只保留生产者元数据，不暴露协议链结或借用正文
    LockFreeHashMap<uint64_t, NetObjectReference<NetSenderSubmissionFront>> m_fronts;
    // 独立回执索引，等待者只读取原子结果和标记消费
    LockFreeHashMap<uint64_t, NetObjectReference<NetSenderSubmissionReceipt>> m_tickets;
    // 统一MPSC入口；同一生产者的请求/流帧/控制帧不得互相越过
    LockFreeQueue<NetSenderSubmissionItem> m_incoming;
    // owner持有已取出的入口前缀；短正文直接供工作区借用，全部退休后才能覆盖
    NetSenderSubmissionItem m_inlineItems[32];
    // owner当前一批短帧的固定工作区，不增加接纳窗口；上一批全部释放后才复用
    NetSenderSubmissionEntry m_inlineEntries[32];
    // 当前仍在FIFO或借用中的短帧工作项，由owner独占
    uint32_t m_inlineOutstanding;
    // 未转换为工作记录的纯DATA前缀长度，0表示未启用
    uint32_t m_directCount;
    // 该前缀中下一项的位置，只由owner访问
    uint32_t m_directIndex;
    // 当前项最后尝试的poll轮次，背压不得在同轮重复发送
    uint64_t m_directAttempt;
    // 外部回调正在借用当前短正文，断线清理不得擦除它
    bool m_directBorrowed;
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
    // owner回执链；未消费回执由物理池持有，已消费项在下一次pump摘除。
    NetSenderSubmissionReceipt* m_receipts;
    // 当前已交协议但尚未全链路交付的回执，非拥有。
    NetSenderSubmissionReceipt* m_draining;
};