#pragma once
#include <NetPeer/NetPeerAPI.h>
#include "NetInterface/NetInterfaceAPI.h"
#include "LockFreeHashMap/LockFreeHashMapAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <atomic>
#include <memory>
#include <map>
#include "NetWorkHostPools.h"

// 一个宿主连接的跨线程入口；路由快照保留本对象，owner独占协议和派发状态
struct NetWorkHostMail
{
public:
    /** 初始化不可复用的宿主连接身份
    @param [in] peer 非零宿主连接号
    */
    explicit NetWorkHostMail(uint64_t peer);

    /** 取得非独占发布借用，关闭后立即失败
    @return 是否成功，成功后必须leave
    */
    bool enter();

    /** 完成队列发布后归还，不等待其他生产者
    */
    void leave();

public:
    // 不变连接身份
    uint64_t m_peer;
    // 高位禁止新输入，低31位为仍可能发布输入的回调数
    std::atomic<uint32_t> m_calls;
    // 已预留且owner尚未导入的事件数，含正在复制/发布的项；终态必须等其归零
    std::atomic<uint64_t> m_pending;
    // 原始连接通知已经发生，owner据此先通告连接再转交输入
    std::atomic<bool> m_connected;
    // 合并尚未导入owner的可写提示
    std::atomic<bool> m_writable;
    // 高32位非零表示原始关闭，低32位保存首次原因，包括0
    std::atomic<uint64_t> m_closed;
    // 仅本连接首次接纳失败，独立于数据队列容量
    std::atomic<int32_t> m_failure;
    // owner已交付终态，旧快照只能退出，不能再次创建同一连接
    std::atomic<bool> m_retired;
};

// 原生事件输入：0连接，1字节，2可写，3关闭；只拷贝原始输入
struct NetWorkHostEvent
{
public:
    // 正文占用额度的归属，跨线程入口与owner槽共同保留至消费，不形成引用环
    NetObjectReference<NetWorkHostMail> m_mail;
    // 事件种类
    uint32_t m_kind;
    // 不复用的宿主连接号
    uint64_t m_peer;
    // 终态原因
    int32_t m_reason;
    // owner独占FIFO下一项，nullptr表示尾部
    NetWorkHostEvent* m_next;
    // 本对象的池槽所有权，仅出队后的owner归还一次
    void* m_storage;
    // 本槽独占输入长度，最多16384字节
    uint32_t m_size;
    // 输入只复制一次，派发时转交独占所有权
    NetBufferLease m_bytes;

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

// 每个连接的owner独占派发状态；跨线程回调只访问NetWorkHostMail
struct NetWorkHostPeer
{
public:
    // 非零活动身份；终态交付前删除，不保留无界墓碑
    uint64_t m_id;
    // 本连接FIFO首项，nullptr表示空；仅owner访问
    NetWorkHostEvent* m_head;
    // 本连接FIFO尾项，nullptr表示空
    NetWorkHostEvent* m_tail;
    // 实际连接池槽，独立于正文和输入队列容量
    NetObjectReference<NetWorkHostMail> m_mail;
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
    @param [in] pools 构造时复制的实际存储配置
    */
    NetWorkHostBridge(const NetHostFunctions& functions, void* observer, NetHostConnected connected, NetHostData data, NetHostWritable writable, NetHostClosed closed, const NetWorkHostPools& pools);

    /** 宿主确认全部回调退出后，释放未派发入口及owner槽；不在这里销毁宿主
    */
    ~NetWorkHostBridge();

    /** 注册完整原生宿主；只能在任何实例／执行者启动之前调用
    @param [in] functions 完整v1表，context借用至全部实例释放
    @return OK或明确参数／配置错误
    */
    static int32_t configure(const NetHostFunctions* functions);

    /** 设置后续宿主实例的实际存储池；生命周期owner串行调用，存在实例时拒绝
    @param [in] pools 各底层组件池配置，失败保持原配置
    @return NI_OK或配置/内存/生命周期错误
    */
    static int32_t configurePools(const NetWorkHostPools& pools);

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

    /** 查找或有界创建不可变连接入口，回调/owner均可调用
    @param [in] peer 非零身份
    @param [in] create 是否允许创建，迟到数据/可写禁止创建
    @return 独立强引用，失败为空
    */
    NetObjectReference<NetWorkHostMail> mailbox(uint64_t peer, bool create);

    /** owner按新增提示导入连接索引；每轮检查已有mail的连接/失败/终态并导入输入
    */
    void pumpMail();

    /** owner撤销连接入口；旧快照保持对象有效但不能再派发
    @param [in] peer 即将交付终态的连接号
    */
    void retireMail(uint64_t peer);

    /** 由owner查找活动连接
    @param [in] peer 非零连接号
    @return owner状态或nullptr；只借用至下次删除此连接
    */
    NetWorkHostPeer* find(uint64_t peer);

    /** 由owner登记已占有实际连接池槽的活动连接
    @param [in] mail 有效连接池引用
    @return 既有或新状态，分配失败为nullptr并记录宿主故障
    */
    NetWorkHostPeer* insert(const NetObjectReference<NetWorkHostMail>& mail);

    /** 由owner归还本连接全部输入槽，安全擦除未交付字节
    @param [in,out] peer 活动连接
    */
    void discard(NetWorkHostPeer& peer);

    /** 由owner记录首次接纳失败；终态另有位置，不分配内存
    @param [in,out] peer 活动连接
    @param [in] reason 负数原因
    */
    void fail(NetWorkHostPeer& peer, int32_t reason);

    /** 只在owner轮流交付有界事件，保持每peer顺序
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
    // 已验证的后续实例配置，启动前串行替换
    static std::unique_ptr<NetWorkHostPools> m_pools;
    // 实际存活的桥数量，禁止配置覆盖存活实例
    static std::atomic<size_t> m_instances;
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
    // 构造期实际池初始化结果
    int32_t m_poolStatus;
    // 实际正文池，晚于所有事件及回调借用销毁
    NetBufferPool m_bodies;
    // 连接对象池，晚于索引、owner状态及输入引用销毁
    NetObjectPool<NetWorkHostMail> m_mailPool;
    // 输入记录池，晚于跨线程队列和owner队列销毁
    NetObjectPool<NetWorkHostEvent> m_records;
    // 无锁连接索引；数据/可写不创建未知入口
    LockFreeHashMap<uint64_t, NetObjectReference<NetWorkHostMail>> m_mailboxes;
    // 独占池记录指针，成功发布后生产者不再访问记录
    LockFreeQueue<NetWorkHostEvent*> m_incoming;
    // 新mail发布后置位，owner先交换清零再扫描
    std::atomic<bool> m_mailDirty;
    // owner独占，按连接号通告和轮询；每节点对应真实mail池槽，不另设连接数门槛
    std::map<uint64_t, NetWorkHostPeer> m_peers;
    // 尚未通告的连接数量，仅统计而非接纳限制
    size_t m_announcements;
    // 连续通告工作量，达到32后给已通告连接一次交付机会
    uint32_t m_announcementBurst;
    // 下轮优先查找大于此号的连接，避免暂停或繁忙连接饿死其他连接
    uint64_t m_cursor;
    // 原始connect调用尚未返回，可接纳同步失败的关闭事件
    std::atomic<bool> m_connecting;
    // owner已请求停止，拒绝新增连接和输入，仍接纳已知连接终态
    std::atomic<bool> m_stopping;
    // 原始shutdown已成功接纳，故障收敛和重复停止不再重复提交
    bool m_shutdownAccepted;
    // 第一次输入失败，回调原子发布、owner读取
    std::atomic<int32_t> m_fault;
    // owner已经向所有已知peer报告故障
    bool m_faultReported;
    // 防止poll／回调销毁重入，仅owner访问
    bool m_polling;
};