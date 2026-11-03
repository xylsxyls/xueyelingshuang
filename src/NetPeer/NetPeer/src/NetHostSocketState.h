#pragma once
#include "NetHostSocket.h"
#include <memory>
#include <map>
#include <vector>

// 内部事件种类不对外公开或跨C ABI传递
enum NetHostEventKind
{
    // 原始连接建立
    NET_HOST_CONNECTED,
    // 一块暂停后的输入
    NET_HOST_DATA,
    // 可合并的本地写完成
    NET_HOST_WRITABLE,
    // 唯一终态
    NET_HOST_CLOSED
};

// 显式的宿主生命周期，停止请求和已经停止不能混为同一状态。
enum NetHostSocketPhase
{
    // 尚未创建原始宿主实例
    NET_HOST_IDLE,
    // 正在调用宿主create，允许同步回调但尚未开始网络操作
    NET_HOST_CREATING,
    // 正在调用listen或connect，结果尚未提交
    NET_HOST_STARTING,
    // 宿主已启动，可收发并轮询事件
    NET_HOST_ACTIVE,
    // 已请求停止，仍需推进关闭回调和宿主回收
    NET_HOST_STOPPING,
    // 宿主已完全回收，不再接纳任何事件
    NET_HOST_STOPPED
};

// 每个peer只保留一个未交付输入块，可写事件合并
struct NetHostSocketPeer
{
public:
    // 原始连接是否已经建立
    bool m_connected;
    // 上层是否已经收到连接建立事件
    bool m_announced;
    // 是否已有输入待交付，阻止不遵守暂停的宿主无限积压
    bool m_inputPending;
    // 是否已有可写事件待交付
    bool m_writablePending;
    // 是否已请求关闭，不再交付后续数据
    bool m_closing;
    // 宿主是否已经交付终态
    bool m_closed;
    // 原始输入当前是否暂停
    bool m_paused;
    // 首个主动关闭原因
    int32_t m_reason;
    // 原始宿主诊断，仅用于报告，不能解释为NI协议结果
    int32_t m_detail;
    // 只保存一块尚未交付的原始输入，最多16KiB；与活动回调缓冲区交换。
    std::vector<uint8_t> m_input;

public:
    /** 初始化未建立的独占连接状态
    */
    NetHostSocketPeer();

    /** 擦除仍未交付的字节，不使宿主输入残留在回收内存中
    */
    ~NetHostSocketPeer();
};

// 固定事件只保存标量，不分配内存；输入由对应端点独占。
struct NetHostSocketEvent
{
    // 本地事件种类
    NetHostEventKind m_kind;
    // 当前实例内不复用代次
    uint64_t m_peer;
    // 终态原因，其他事件0
    int32_t m_reason;
    // 原始宿主诊断，非关闭事件为0
    int32_t m_detail;
};

// 同一执行者拥有整个状态；宿主上下文保留至成功destroy
class NetHostSocketState
{
public:
    /** 复制函数表，尚不创建宿主
    @param [in] functions 完整有效或全零的函数表
    */
    explicit NetHostSocketState(const NetHostFunctions& functions);

    /** 检查完整v1表，不读取短结构的函数项
    @param [in] functions 调用方有效结构
    @return 是否全部必需项存在
    */
    static bool valid(const NetHostFunctions& functions);

    /** 接纳一次启动并建立宿主，失败不保留上层监听者
    @param [in] listener 借用至停止完成
    @param [in] maxPeers 已验证连接上限
    @param [in] server 是否监听
    @return OK或负数
    */
    int32_t start(NetHostSocketListener* listener, uint32_t maxPeers, bool server);

    /** 非阻塞驱动，事件只在此层交付；不允许重入
    @return 是否仍有资源
    */
    bool poll();

    /** 请求关闭一代，不重复分配终态
    @param [in] peer 当前连接
    @param [in] reason 首次原因
    */
    void close(uint64_t peer, int32_t reason);

    /** 停止宿主并收敛事件，在外层执行者上调用
    */
    void stop();

    /** 记录首次宿主违约，外层驱动将统一关闭
    @param [in] reason 负数错误
    */
    void fail(int32_t reason);

    /** 无分配地排队一个事件；每个端点最多连接/数据/可写/终态四项
    @param [in] kind 事件种类
    @param [in] peer 代次
    @param [in] reason 终态原因
    @param [in] detail 宿主诊断
    @return 是否接纳，满时记录持续错误
    */
    bool queue(NetHostEventKind kind, uint64_t peer, int32_t reason, int32_t detail = 0);

    /** 擦除活动回调输入，保留最多16KiB供下次交换
    */
    void clearInput();

    /** 排空有界事件并删除已交付终态的peer
    */
    void dispatch();

    /** 接收宿主原始连接，先暂停再排队
    @param [in] context 稳定独占状态
    @param [in] peer 不复用连接号
    */
    static void connected(void* context, uint64_t peer);

    /** 复制一块输入，先暂停宿主
    @param [in] context 稳定独占状态
    @param [in] peer 当前连接
    @param [in] data 借用字节
    @param [in] size 1至16384
    */
    static void data(void* context, uint64_t peer, const uint8_t* data, uint32_t size);

    /** 合并本地写完成通知
    @param [in] context 稳定独占状态
    @param [in] peer 当前连接
    */
    static void writable(void* context, uint64_t peer);

    /** 排队唯一终态，允许未建立的异步连接直接失败
    @param [in] context 稳定独占状态
    @param [in] peer 当前连接
    @param [in] reason 宿主错误
    */
    static void closed(void* context, uint64_t peer, int32_t reason);

public:
    // 进程启动时唯一写入，创建工作执行者后只读
    static NetHostFunctions m_default;
    // 默认宿主成功配置后不允许更换
    static bool m_configured;
    // 完整复制的宿主表；context借用
    NetHostFunctions m_functions;
    // 宿主独占实例，只由functions.destroy释放
    void* m_instance;
    // 借用至停止完成的监听者
    NetHostSocketListener* m_listener;
    // 连接上限内的状态
    std::map<uint64_t, NetHostSocketPeer> m_peers;
    // 启动时一次性分配maxPeers*4个标量槽，收发期间不扩容。
    std::unique_ptr<NetHostSocketEvent[]> m_events;
    // 启动时确定的标量环形队列容量，不在收发期间扩容
    uint32_t m_eventCapacity;
    // 下一个待读取标量事件的环形槽下标
    uint32_t m_eventHead;
    // 尚未派发的标量事件数，不超过m_eventCapacity
    uint32_t m_eventCount;
    // 当前唯一回调借用的输入，避免恢复读取时同步新输入覆盖它。
    std::vector<uint8_t> m_input;
    // 见过的最大代次，旧事件无需永久墓碑表
    uint64_t m_lastPeer;
    // 当前排队输入总字节，最大4MiB
    uint32_t m_queuedBytes;
    // 配置连接上限1至256
    uint32_t m_maxPeers;
    // 首个全实例错误
    int32_t m_fault;
    // 创建、启动、运行、请求停止及停止完成的唯一状态。
    NetHostSocketPhase m_phase;
    // 监听角色
    bool m_server;
    // 是否正在驱动或结算停止
    bool m_polling;
    // 是否正在上层回调
    bool m_callback;
};