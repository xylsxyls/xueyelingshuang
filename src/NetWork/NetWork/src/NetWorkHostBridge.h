#pragma once
#include <NetPeer/NetPeerAPI.h>
#include "NetInterface/NetInterfaceAPI.h"
#include <NetInterface/NetInterfaceAPI.h>
#include <ReadWriteMutex/ReadWriteMutexAPI.h>
#include <atomic>
#include <memory>

// 原生事件输入：0连接，1字节，2可写，3关闭；只拷贝原始输入
struct NetWorkHostEvent
{
public:
    // 事件种类
    uint32_t m_kind;
    // 不复用的宿主连接号
    uint64_t m_peer;
    // 终态原因
    int32_t m_reason;
    // 下一队列槽或空闲槽，2048表示尾部
    uint32_t m_next;
    // 本槽独占输入长度，最多16384字节
    uint32_t m_size;
    // 输入只复制一次，派发时转交独占所有权
    std::unique_ptr<uint8_t[]> m_bytes;

public:
    /** 初始化空标量槽，不分配正文
    */
    NetWorkHostEvent();

    /** 擦除尚未交付的输入
    */
    ~NetWorkHostEvent();

    /** 擦除并释放输入，不改变槽间链接
    */
    void clear();
};

// 每个连接的提交门下状态；协议不访问这些字段
struct NetWorkHostPeer
{
public:
    // 0表示空槽；终态交付后归还，不保存已关闭连接的无界墓碑
    uint64_t m_id;
    // 本连接FIFO的首尾槽，2048表示空
    uint32_t m_head;
    // 本连接FIFO尾槽，2048表示空；由m_mutex保护
    uint32_t m_tail;
    // 原始连接事件已接纳，重复事件不占队列
    bool m_connected;
    // owner已按连接号顺序通告；通告独立于数据槽和轮询游标。
    bool m_announced;
    // owner已要求暂停；排队输入等待恢复后再交付
    bool m_paused;
    // 合并宿主可写通知
    bool m_writable;
    // 原始宿主已报告终态，排队尾部仍须先于关闭通知交付，不再操作原始socket
    bool m_closed;
    // 仅本连接接纳失败，不使健康连接失效
    bool m_failed;
    // 正常终态或首次接纳失败原因
    int32_t m_reason;

public:
    /** 初始化可复用的空连接槽
    */
    NetWorkHostPeer();
};

// 原生异步事件只在poll执行者交给Peer；不实现网络或任何协议
class NetWorkHostBridge
{
public:
    /** 复制完整提供方表，借用观察者及其上下文至destroy成功
    @param [in] functions 已验证v1表
    @param [in] observer 观察者上下文
    @param [in] connected 连接事件
    @param [in] data 字节事件
    @param [in] writable 可写事件
    @param [in] closed 终态事件
    */
    NetWorkHostBridge(const NetHostFunctions& functions, void* observer, NetHostConnected connected, NetHostData data, NetHostWritable writable, NetHostClosed closed);

    /** 注册完整原生宿主；只能在任何实例／执行者启动之前调用
    @param [in] functions 完整v1表，context借用至全部实例释放
    @return OK或明确参数／配置错误
    */
    static int32_t configure(const NetHostFunctions* functions);

    /** 复制宿主表并创建桥接器；部分创建失败时先停用回调再回收宿主
    @param [in] provider 完整NetHostFunctions表
    @param [in] observer 借用的观察者上下文
    @param [in] connected 连接回调
    @param [in] data 字节回调
    @param [in] writable 可写回调
    @param [in] closed 关闭回调
    @param [out] instance 成功时独占桥接器；失败通常为空，部分宿主尚未收敛时仅供重试shutdown／destroy，禁止业务操作
    @return 0成功；宿主错误或异常为负数
    */
    static int32_t create(void* provider, void* observer, NetHostConnected connected, NetHostData data, NetHostWritable writable, NetHostClosed closed, void** instance);

    /** 开始监听，所属owner调用
    @param [in] instance 有效桥接器
    @param [in] address 数值地址
    @param [in] port 监听端口
    @param [in] maxPeers 最大连接数
    @return 原始宿主结果；异常为-1
    */
    static int32_t listen(void* instance, const char* address, uint32_t port, uint32_t maxPeers);

    /** 创建连接并登记独立事件状态
    @param [in] instance 有效桥接器
    @param [in] address 数值地址
    @param [in] port 目标端口
    @param [out] peer 宿主提供的连接号
    @return 原始宿主结果；异常为-1
    */
    static int32_t connect(void* instance, const char* address, uint32_t port, uint64_t* peer);

    /** 只由owner推进宿主及有界事件；禁止回调内嵌套推进
    @param [in] instance 有效桥接器
    @return 1仍需推进，0静止，负数失败；异常不越过C表
    */
    static int32_t poll(void* instance);

    /** 由宿主完整接纳字节，输入仅借用至返回
    @param [in] instance 有效桥接器
    @param [in] peer 连接号
    @param [in] bytes 字节首地址
    @param [in] size 字节数
    @return 原始宿主结果；异常为-1
    */
    static int32_t write(void* instance, uint64_t peer, const uint8_t* bytes, uint32_t size);

    /** 调整单连接读取，不改变其他连接的推进
    @param [in] instance 有效桥接器
    @param [in] peer 连接号
    @param [in] paused 非零暂停，0恢复
    @return 原始宿主结果；异常为-1
    */
    static int32_t pauseRead(void* instance, uint64_t peer, int32_t paused);

    /** 请求关闭单连接，不销毁桥接器
    @param [in] instance 有效桥接器
    @param [in] peer 连接号
    @param [in] reason 关闭原因
    @return 原始宿主结果；异常为-1
    */
    static int32_t close(void* instance, uint64_t peer, int32_t reason);

    /** 停止宿主并收敛终态；异常后仍允许owner重试清理
    @param [in] instance 有效桥接器
    @return 原始宿主结果；异常为-1
    */
    static int32_t shutdown(void* instance);

    /** 只在宿主确认销毁后释放桥接器，禁止回调中释放
    @param [in] instance 有效桥接器或空
    @return 0已释放；负数保留有效句柄供owner清理
    */
    static int32_t destroy(void* instance);

    /** 查询本机监听端口
    @param [in] instance 有效桥接器
    @return 原始端口；无监听或宿主异常为0
    */
    static uint32_t localPort(void* instance);

    /** 查询宿主连接数
    @param [in] instance 有效桥接器
    @return 原始数量；宿主异常为0
    */
    static uint32_t peerCount(void* instance);

    /** 从固定提供方取得强随机，失败不得降级
    @param [in] provider 配置了entropy的原始函数表
    @param [out] bytes 输出缓冲
    @param [in] size 请求字节数
    @return 宿主结果；异常为-1
    */
    static int32_t entropy(void* provider, uint8_t* bytes, uint32_t size);

    /** 可跨线程接纳连接事件；只复制有界输入，不触碰Peer
    @param [in] context 原始宿主持有的有效桥接器，至destroy成功前存活
    @param [in] peer 不复用的连接号
    */
    static void connected(void* context, uint64_t peer);

    /** 可跨线程接纳输入字节事件；只复制有界输入，不触碰Peer
    @param [in] context 原始宿主持有的有效桥接器，至destroy成功前存活
    @param [in] peer 不复用的连接号
    @param [in] bytes 回调期间借用；在此复制
    @param [in] size 字节数
    */
    static void data(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size);

    /** 可跨线程接纳可写事件；只复制有界输入，不触碰Peer
    @param [in] context 原始宿主持有的有效桥接器，至destroy成功前存活
    @param [in] peer 不复用的连接号
    */
    static void writable(void* context, uint64_t peer);

    /** 可跨线程接纳关闭事件；只复制有界输入，不触碰Peer
    @param [in] context 原始宿主持有的有效桥接器，至destroy成功前存活
    @param [in] peer 不复用的连接号
    @param [in] reason 关闭原因
    */
    static void closed(void* context, uint64_t peer, int32_t reason);
private:
    /** 有界接纳原始事件；已知连接过载只使本连接失败
    @param [in] kind 0至3
    @param [in] peer 本实例连接号
    @param [in] reason 终态原因
    @param [in] bytes 调用时借用
    @param [in] size 字节数
    */
    void enqueue(uint32_t kind, uint64_t peer, int32_t reason, const uint8_t* bytes, uint32_t size);

    /** 在锁内查找活动连接
    @param [in] peer 非零连接号
    @return 槽位或256表示不存在
    */
    uint32_t find(uint64_t peer) const;

    /** 在锁内登记活动连接，禁止超过256个
    @param [in] peer 非零连接号
    @return 既有或新槽位，256表示满
    */
    uint32_t insert(uint64_t peer);

    /** 在锁内归还本连接全部输入槽，安全擦除未交付字节
    @param [in,out] peer 活动连接
    */
    void discard(NetWorkHostPeer& peer);

    /** 在锁内记录首次接纳失败；终态另有位置，不分配内存
    @param [in,out] peer 活动连接
    @param [in] reason 负数原因
    */
    void fail(NetWorkHostPeer& peer, int32_t reason);

    /** 只在owner锁外轮流交付有界事件，保持每peer顺序
    */
    void drain();

    /** 禁止复制宿主所有权
    @param [in] other 不允许复制的实例
    */
    NetWorkHostBridge(const NetWorkHostBridge& other);

    /** 禁止赋值宿主所有权
    @param [in] other 不允许赋值的实例
    @return 本对象引用，仅声明以禁止赋值
    */
    NetWorkHostBridge& operator=(const NetWorkHostBridge& other);
private:
    // 启动时只读配置；只允许一次成功注册
    static NetHostFunctions m_provider;
    // 启动配置不和实例创建并发
    static bool m_configured;
    // 本实例固定提供方表
    NetHostFunctions m_functions;
    // 原始字节宿主，直至其destroy成功前始终保留
    void* m_instance;
    // 借用的Peer观察者上下文
    void* m_observer;
    // 观察者函数只由owner调用
    NetHostConnected m_connected;
    // 输入观察者，只由poll执行者调用，输入借用至回调返回
    NetHostData m_data;
    // 可写观察者，只由poll执行者调用，可写通知按连接合并
    NetHostWritable m_writable;
    // 终态观察者，只由poll执行者在已接纳输入排空后调用
    NetHostClosed m_closed;
    // 仅保护原始事件／复制体及本地暂停状态
    ReadWriteMutex m_mutex;
    // 固定2048个槽，无deque移动、扩容或收发期间节点分配
    NetWorkHostEvent m_events[2048];
    // 固定256个连接槽；关闭标量独立于满数据队列
    NetWorkHostPeer m_peers[256];
    // 空闲槽链首，2048表示已满
    uint32_t m_free;
    // 活动连接数，连接接纳和主动connect都检查同一上限
    uint32_t m_peerCount;
    // 尚未通告的新连接数；为0时热路径不额外扫描连接表。
    uint32_t m_announcements;
    // 下次派发优先位置，防止暂停或繁忙连接饿死其他连接
    uint32_t m_cursor;
    // 排队输入预算
    uint32_t m_bytes;
    // 原始connect调用尚未返回，可接纳同步失败的关闭事件
    bool m_connecting;
    // owner已请求停止，拒绝新增连接和输入，仍接纳已知连接终态
    bool m_stopping;
    // 原始shutdown已成功接纳，故障收敛和重复停止不再重复提交
    bool m_shutdownAccepted;
    // 第一次输入失败，锁内写、owner读取
    std::atomic<int32_t> m_fault;
    // owner已经向所有已知peer报告故障
    bool m_faultReported;
    // 防止poll／回调销毁重入，仅owner访问
    bool m_polling;
};