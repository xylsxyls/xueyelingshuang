#pragma once
#include "uv.h"
#include "LibuvPoolSettings.h"
#include "LockFreeHashMap/LockFreeHashMapAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <stdint.h>
#include <atomic>
#include <unordered_map>

/** 固定线程拥有的借用计数，关闭线程只读取，不修改拥有者计数
*/
struct LibuvLifecycleSlot
{
public:
    // 只分配一次且线程令牌不复用；冲突线程使用公共后备计数
    std::atomic<uint32_t> m_token;
    // 所属线程的嵌套借用数，进入和关闭以顺序一致操作建立先后关系
    std::atomic<uint32_t> m_calls;
    // 分隔不同发送线程写入的计数
    char m_padding[120];

public:
    /** 创建未分配的零借用槽
    */
    LibuvLifecycleSlot();
};

/** 上下文的并发借用计数；关闭只等待已经进入的调用，不以独占锁串行发送
*/
class LibuvLifecycleGate
{
public:
    /** 创建关闭状态；initialize完成后由生命周期owner开放
    */
    LibuvLifecycleGate();

    /** 检查本平台是否提供原生无锁32位计数
    @return 是否可用
    */
    bool valid() const;

    /** owner在全部资源准备完成后开放，必须处于关闭且无人借用状态
    */
    void open();

    /** 取得一个非独占借用；关闭或计数耗尽立即失败，不等待其他借用者
    @return 成功后必须release一次
    */
    bool acquire();

    /** 尝试取得借用，区分竞争、计数容量和关闭状态
    @param [in] retryContention 是否在 CAS 竞争时持续重试
    @return 0 成功，后备计数CAS竞争可返回UV_EAGAIN，计数耗尽UV_EOVERFLOW，关闭UV_ECANCELED
    */
    int32_t tryAcquire(bool retryContention);

    /** 由取得借用的同一线程结束本次借用，此后不再触碰上下文或uv唤醒句柄
    */
    void release();

    /** owner封闭新借用并等待旧调用退出；仅回收阶段允许等待，不从借用中调用
    */
    void close();
private:
    // 高位为关闭标志，低31位为无法领取线程槽时的后备借用计数
    std::atomic<uint32_t> m_calls;
    // 关闭标志不与发送线程计数共用缓存行
    char m_padding[124];
    // 槽拥有者在对象整个生命期内不变，open不清零可能迟到的失败借用
    LibuvLifecycleSlot m_slots[32];
};

// 一个方法调用的非独占借用；多个send可同时持有，不能复制或提前归还
class LibuvCall
{
public:
    /** 在读取上下文指针之前取得借用
    @param [in,out] gate 外层对象拥有且覆盖整个调用的计数器
    @param [in] retryContention 是否持续重试入口 CAS，默认用于普通内部调用
    */
    explicit LibuvCall(LibuvLifecycleGate& gate, bool retryContention = true);

    /** 方法不再访问上下文后归还借用
    */
    ~LibuvCall();

    /** 是否已取得当前生命周期借用
    @return 本次接纳结果
    */
    bool accepted() const;

    /** 查询取得借用的完整状态，不丢失竞争与关闭的区别
    @return 0 成功，其他为 libuv 错误码
    */
    int32_t status() const;
private:
    /** 禁止复制计数所有权
    @param [in] other 原借用
    */
    LibuvCall(const LibuvCall& other);

    /** 禁止重复归还借用
    @param [in] other 原借用
    @return 不可调用
    */
    LibuvCall& operator=(const LibuvCall& other);
private:
    // 构造时已存在的外层计数器
    LibuvLifecycleGate& m_gate;
    // 只有成功取得时才在析构归还
    int32_t m_status;
};

// 排队时保存连接代次，消费时不凭可能已经释放/复用的TCP裸地址发送。
struct LibuvPendingSend
{
public:
    // 拥有的大帧或批量数组，消费或取消后在本库释放；内联帧和关闭操作为空
    char *m_buffer;
    // 仅作登记表查询键的TCP借用地址，发送前须同时核对代次
    uv_tcp_t *m_target;
    // 提交时记录的连接代次，防止旧地址复用导致串线
    uint64_t m_generation;
    // true表示关闭目标连接，false表示提交数据
    bool m_close;
    // 已预留的发送预算，覆盖数组及写请求；消费或写完成后恰好归还一次
    uint32_t m_charge;
    // 非零为明确的线上字节数；零从数组读取长度，空数组的有效内联长度为4至16
    uint32_t m_wireSize;
    // 单帧线上长度至多16字节时内联持有；m_buffer为空且m_wireSize非零表示有效
    char m_inlineBytes[16];

public:
    /** 初始化空操作，不拥有TCP句柄
     */
    LibuvPendingSend();
};

/** 固定发送线程的有界单写单读通道，实例主通道与附加生产者通道复用此实现
*/
struct LibuvSendLane
{
public:
    // 首次领取后不换主，令牌耗尽时不用此通道
    std::atomic<uint32_t> m_token;
    // 写者发布的末端，读者通过acquire取得完整描述符
    std::atomic<uint32_t> m_published;
    // 仅所属发送线程使用的下一个写入序号
    uint32_t m_write;
    // 写者缓存的已消费前缀，空间不足时才重新读取
    uint32_t m_seenRead;
    // 分隔写者与读者频繁修改的索引
    char m_beforeRead[128];
    // 读者完成复制后发布，写者才允许覆盖对应槽
    std::atomic<uint32_t> m_consumed;
    // 仅循环owner使用的读取序号
    uint32_t m_read;
    // 分隔索引与数据，避免描述符复制触碰另一端索引缓存行
    char m_beforeItems[128];
    // 按初始化配置分配，运行期间不扩容；零容量时禁用本通道
    LibuvPendingSend* m_items;
    // 固定容量，0或2的幂；用于索引回绕
    uint32_t m_capacity;
    // 指针和容量为两端只读，避免紧邻外层消费轮转标志而产生伪共享
    char m_afterItems[64];

public:
    /** 初始化空通道，序号用无符号回绕差值计算容量
    @param [in] capacity 0或2的幂，配置已经通过外层检查
    */
    explicit LibuvSendLane(uint32_t capacity = 1024);

    /** 上层已取完待发送数据后释放描述符存储，不拥有残留数组指针
    */
    ~LibuvSendLane();

    /** 仅所属发送线程发布一项，不等待容量，不转移失败项的所有权
    @param [in] pending 已预留预算的完整描述符
    @return 是否发布成功
    */
    bool push(const LibuvPendingSend& pending);

    /** 仅循环owner复制已发布前缀，再归还可覆盖槽
    @param [out] output 至少maximum项的暂存空间
    @param [in] maximum 本轮读取上限
    @return 实际复制的条数
    */
    size_t pop(LibuvPendingSend* output, size_t maximum);
};

class LibuvTcp;
struct LibuvBase;

/** 实例内有界缓存的生产者上下文，线程令牌不复用；仅所属线程调用，静止销毁时统一释放
*/
struct LibuvSendProducer
{
public:
    // 零为空，非零一经领取在本实例生命周期内不再换主，其他线程走普通队列入口
    std::atomic<uint32_t> m_token;
    // 仅所属线程读写；同线程重入时绕开正在使用的上下文，不用于线程间互斥
    bool m_busy;
    // 独占通道禁用时借用队列登记，销毁队列前必须先release
    LockFreeQueueProducer<LibuvPendingSend> m_producer;
    // 固定线程独占的附加通道，初始化时分配；满载仍留在本通道以保持线程内顺序
    LibuvSendLane m_lane;
    // 分隔不同生产者频繁修改的字段，避免相邻缓存槽共享缓存行
    char m_padding[64];

public:
    /** 创建尚未分配给任何线程的缓存槽
    */
    LibuvSendProducer();
};

/** 仅循环 owner 复用的写请求，回调之前独占正文和预算
*/
struct LibuvPendingWrite
{
public:
    // 交给 libuv 的实际写请求
    uv_write_t m_request;
    // 覆盖写完成或取消回调的循环 owner
    LibuvBase* m_owner;
    // 写完成前拥有的数组
    char* m_buffer;
    // 当前数组及请求的预算
    uint32_t m_charge;
    // 空闲缓存链，不与生产者共享
    LibuvPendingWrite* m_next;
    // 内联小帧在异步写完成前由请求持有，不借用消费栈
    char m_inlineBytes[16];

    /** 初始化空请求，尚未提交给 libuv
    */
    LibuvPendingWrite();
};

/** 一次读回调的缓冲租约；不同未完成读取持有不同数组，避免跨连接覆盖
*/
class LibuvReadCall
{
public:
    /** 接管 allocateRead 配对交付的数组
    @param [in,out] owner 回调期间存活的循环状态
    @param [in] bytes 本次数组，可空
    */
    LibuvReadCall(LibuvBase& owner, char* bytes);

    /** 回调结束时归还给有界缓存；不传播异常
    */
    ~LibuvReadCall();
private:
    /** 禁止复制缓冲租约
    @param [in] other 原租约
    */
    LibuvReadCall(const LibuvReadCall& other);

    /** 禁止重复归还数组
    @param [in] other 原租约
    @return 不可调用
    */
    LibuvReadCall& operator=(const LibuvReadCall& other);
private:
    // 单 owner 循环状态，覆盖全部读回调
    LibuvBase& m_owner;
    // 本次独占的缓冲，析构时归还
    char* m_bytes;
};

typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_loop_s uv_loop_t;
typedef struct uv_async_s uv_async_t;
typedef struct uv_connect_s uv_connect_t;
template <class QElmType> class LockFreeQueue;

/** 兼容接口的循环资源所有者；循环执行者关闭句柄，提交线程仅借用无锁登记快照并发布队列
*/
struct LibuvBase
{
public:
    // 初始化时复制的固定配置，运行期间只读
    const LibuvPoolSettings m_settings;
    // 初始化后只读的缓存地址，远离owner消费暂存区及首线程发布序号
    LibuvSendProducer* m_sendProducers;
    // 容量为2次幂时用掩码定位，否则UINT32_MAX表示使用普通取模
    const uint32_t m_producerMask;
    // 借用的外层宿主，存活至全部uv回调及排空结束
    LibuvTcp *m_libuvTcp;
    // 拥有的私有循环，成功初始化后才能运行和关闭
    uv_loop_t *m_loop;
    // 发送队列唤醒句柄，允许多生产者提交后唤醒唯一循环
    uv_async_t *m_asyncHandle;
    // 构造期有界队列池可用性，initialize在失败时拒绝创建uv资源
    bool m_queueReady;
    // 拥有的待发送队列，停止时消费并回收剩余数组
    LockFreeQueue<LibuvPendingSend> *m_queue;
    // 取消专用队列；关闭不被网络写窗口阻塞，也不承诺刷新尚未发送的数据
    LockFreeQueue<LibuvPendingSend> *m_closeQueue;
    // 停止专用唤醒句柄，避免关闭请求依赖新的队列分配
    uv_async_t *m_asyncCloseHandle;
    // 派生上下文类型标记，true为客户端，false为服务端
    bool m_isClient;

    // 地址转无损整数键以使用容器的标量副本查询；消费端仍核对不复用代次
    LockFreeHashMap<uintptr_t, uint64_t> m_connections;
    // 循环 owner 专用登记；消费端无需重复申请并发容器读保护，业务不跨线程访问
    std::unordered_map<uv_tcp_t*, uint64_t> m_ownerConnections;
    // 允许提交线程请求停止，句柄只由循环线程关闭
    std::atomic<bool> m_stopping;
    // 下次登记的单调连接代次，耗尽后停止接纳而不复用
    uint64_t m_nextConnection;
    // 循环初始化成功，排空后才能调用uv_loop_close
    bool m_loopInitialized;
    // 循环执行者已开始幂等关闭，禁止再次提交读写
    bool m_closing;
    // 按读池配置创建的空闲数组目录，在途数组不在此表中
    char** m_readBuffers;
    // 当前空闲数组数，仅由循环执行者更新
    uint32_t m_cachedReads;
    // 分隔只由owner写入的读缓存与跨线程预算，避免共享缓存行
    char m_beforeBudget[64];
    // 发送者预留，取消或完成后归还；成功接纳不超过配置预算，64位容纳并发失败预留的临时加数
    std::atomic<uint64_t> m_pendingBytes;
    // 分隔owner写请求状态与跨线程预算；不参与同步或容量计算
    char m_afterBudget[64];
    // owner专用的空写请求链，数量由发送池配置限制
    LibuvPendingWrite* m_freeWrites;
    // owner 当前空写请求数
    uint32_t m_cachedWrites;
    // owner已交给uv_write且尚未回调的请求数，不超过配置写窗口
    uint32_t m_activeWrites;
    // loop 启动时写入一次，发布 m_ownerReady 后不再改变
    uv_thread_t m_ownerThread;
    // release 发布循环执行者身份，其他线程读取前 acquire
    std::atomic<bool> m_ownerReady;
    // 唯一loop复用的取出暂存区；每次只读取实际取出的前缀，处理后旧指针不再拥有数组
    LibuvPendingSend m_pendingScratch[256];
    // 首个发送线程专用，所有发送通道共享配置的字节预算；可设容量0禁用
    LibuvSendLane m_sendLane;
    // 唯一消费线程在主通道与并行入口组之间轮换，组内再轮转，持续单线程流量不饿死其他发送者
    bool m_laneFirst;
    // 附加通道与共享队列的轮转位置，仅网络循环读写
    uint32_t m_parallelCursor;

public:
    /** 创建空的循环、队列及唤醒存储，实际uv初始化由initialize完成
    @param [in] settings 已检查的完整配置，构造时复制
    */
    explicit LibuvBase(const LibuvPoolSettings& settings = LibuvPoolSettings());

    /** 所有uv回调结束后释放构造阶段内存
     */
    ~LibuvBase();

    /** 在持有外层生命周期借用时提交一项，复用可用的本线程登记并完成发布屏障
    @param [in] pending 已经预留预算的完整操作，不修改输入
    @param [in] retryContention 是否在队列CAS竞争时持续重试；容量始终由外层可取消等待
    @return 队列接纳结果，失败不转移数组；缓存不可用不阻止普通队列路径
    */
    LockFreeQueueResult tryQueueSend(const LibuvPendingSend& pending, bool retryContention);

    /** owner公平读取一个已发布前缀，跨线程不承诺全局先后，线程内始终固定通道
    @param [out] output 至少maximum项空间
    @param [in] maximum 上限
    @param [out] taken 本次已取出数量
    @return 是否取到数据
    */
    bool popSendPrefix(LibuvPendingSend* output, size_t maximum, size_t& taken);

    /** 轮转读取已配置的生产者通道和共享队列，任何持续就绪的入口均能推进
    @param [out] output 至少maximum项的暂存区
    @param [in] maximum 本次消费上限
    @param [out] taken 实际取出条数
    @return 是否取到前缀；空通道不阻塞其他通道
    */
    bool popParallelPrefix(LibuvPendingSend* output, size_t maximum, size_t& taken);

    /** owner检查是否仍有已发布数据，用于写窗口恢复和继续唤醒
    @return 任一发送通道可能非空
    */
    bool sendsReady() const;

    /** 获得本线程不复用的库内身份，只保存整数，不持有队列或实例地址
    @return 1至UINT32_MAX-1为有效令牌，UINT32_MAX表示耗尽后使用普通队列路径
    */
    static uint32_t producerToken();

    /** 拷贝至多16字节的已检查小正文，不读取或写入范围以外的字节
    @param [out] target 有效输出空间
    @param [in] source 至少size字节的输入
    @param [in] size 0至16的字节数
    */
    static void copySmall(char* target, const char* source, uint32_t size);

    /** 为尚无令牌的线程分配不复用身份；耗尽时回退普通路径
    @return 有效令牌或UINT32_MAX
    */
    static uint32_t allocateProducerToken();

    /** 初始化循环、唤醒和主TCP，记录每一步的成功状态
    @param [in] owner 借用到loop结束的宿主
    @return 0成功或libuv错误
    */
    int32_t initialize(LibuvTcp *owner);

    /** 取得派生上下文的主TCP
    @return 客户端连接或服务器监听句柄
    */
    uv_tcp_t *mainTcp();

    /** 循环线程登记已连接TCP的新代次
    @param [in] tcp 本循环连接
    @return 是否成功，分配失败请求停止
    */
    bool registerTcp(uv_tcp_t *tcp);

    /** 复制稳定连接代次；查询资源失败则停止本循环，不能当普通不存在而静默丢包
    @param [in] tcp 只作键使用，查到有效代次之前不解引用
    @param [out] generation 有效连接代次
    @return 是否找到有效连接；调用方必须持有生命周期借用或为循环owner
    */
    bool connectionGeneration(uv_tcp_t* tcp, uint64_t& generation);

    /** 循环线程撤销可发送资格并幂等关闭
    @param [in] tcp 已初始化且未释放的自有TCP
    */
    void closeTcp(uv_tcp_t *tcp);

    /** 循环线程请求关闭全部已初始化自有句柄
     */
    void beginClose();

    /** 循环未运行或已返回时推进所有取消/关闭回调再关闭循环
     */
    void drain();

    /** 循环线程开始读
    @param [in] tcp 已连接TCP
    @return 0成功或libuv错误
    */
    int32_t startRead(uv_tcp_t *tcp);

    /** 循环执行者按读池配置取得独占数组；缓存空时按需分配
    @return 缓冲或空；由对应 Read 回调归还
    */
    char* acquireReadBytes();

    /** 循环执行者归还数组，停止或缓存已满时立即释放
    @param [in] bytes 本实例申请的数组，可空，不得重复归还
    */
    void releaseReadBytes(char* bytes);

    /** 通过原子加减预留发送预算，无CAS竞争失败返回
    @param [in] charge 数组和请求开销，须在有效上限内
    @param [in] retryContention 保留统一调用签名，本预留路径无需CAS重试
    @return 0 成功，UV_ENOBUFS 当前预留容量不足，UV_EINVAL 永远无法容纳的请求
    */
    int32_t tryReserveSend(uint32_t charge, bool retryContention);

    /** 判断当前执行者是否正是需要腾出空间的循环线程
    @return true 表示不得在当前栈等待自身循环进度
    */
    bool isLoopOwner() const;

    /** 空间不足时先让出时间片再重试，达到4096次后每轮请求休眠1毫秒
    @param [in,out] attempts 已发生的容量失败次数，饱和在4096避免溢出
    */
    static void WaitForSpace(uint32_t& attempts);

    /** CAS持续竞争时定期让出执行机会；不将内部重试转换为业务Busy
    @param [in,out] attempts 本次调用的竞争计数，每64次让步、每4096次请求休眠1毫秒，允许无符号回绕
    */
    static void WaitForContention(uint32_t& attempts);

    /** 归还已接纳预算，不触碰数组
    @param [in] charge 对应预留，0 表示无预留
    */
    void releaseSend(uint32_t charge);

    /** owner 获取一个未提交的写请求
    @return 独占请求或空
    */
    LibuvPendingWrite* acquireWrite();

    /** owner 释放正文及预算，按有界策略复用请求对象
    @param [in] request 已完成、取消或立即失败的写请求，不可空
    */
    void releaseWrite(LibuvPendingWrite* request);

    /** owner 消费一项已取出的操作，所有路径都交付或释放其资源
    @param [in] pending 独占待发操作，不得再次消费
    @param [in] written 已同步写出的前缀字节，0表示尚未尝试；非零时只异步发送剩余字节
    */
    void processPending(const LibuvPendingSend& pending, uint32_t written = 0);

    /** 顺序消费已弹出的前缀，同连接连续小帧合并一次写入；不等待凑批
    @param [in] pending 独占操作数组
    @param [in] count 本次已弹出数量，最多256
    */
    void processPrefix(const LibuvPendingSend* pending, size_t count);

    /** 尝试直接交付同连接小帧前缀，完整同步写不申请堆缓冲或写请求
    @param [in] pending 已核对同目标同代次的独占操作数组
    @param [in] count 前缀项数，2至256
    @param [in] wireSize 前缀线上总长，不超过16388时尝试快路径
    @param [in] charge 前缀已预留预算合计
    @return true已消费或安全收敛全部资源；false未转移资源，调用者继续普通路径
    */
    bool processDirectPrefix(const LibuvPendingSend* pending, size_t count, uint32_t wireSize, uint32_t charge);

    /** owner 收敛 I/O 失败并通知一次断开；已在关闭中的连接不重复通知
    @param [in] tcp 仍由本循环拥有的连接
    */
    void failTcp(uv_tcp_t* tcp);

    /** 在状态发布之后唤醒循环，兼容旧 libuv 合并唤醒的内存顺序
    @param [in] handle 借用保护内或 owner 持有的有效 async 句柄
    @return libuv 唤醒结果；成功不代表业务数据已经送达
    */
    static int32_t Wake(uv_async_t* handle);

    /** 遍历关闭当前上下文句柄
    @param [in] handle uv_walk借用句柄
    @param [in] context 当前上下文
    */
    static void WalkClose(uv_handle_t *handle, void *context);

    /** 所有请求回调结束后释放accept句柄，主句柄留给上下文
    @param [in] handle 已完成关闭的TCP
    */
    static void TcpClosed(uv_handle_t *handle);

    /** 有界分配接收缓冲，失败返回空缓冲
    @param [in] handle 当前连接
    @param [in] suggestedSize libuv建议
    @param [out] buffer 本次缓冲
    */
    static void AllocateRead(uv_handle_t *handle, size_t suggestedSize, uv_buf_t *buffer);

    /** 接收/断开处理，异常也释放缓冲且不穿过C回调
    @param [in] sender 当前TCP
    @param [in] size 字节数或负数错误
    @param [in] buffer 接收缓冲
    */
    static void Read(uv_stream_t *sender, ssize_t size, const uv_buf_t *buffer);

    /** 处理异步连接完成/取消
    @param [in] request 当前请求
    @param [in] status libuv结果
    */
    static void Connected(uv_connect_t *request, int status);

    /** 接收新TCP，所有失败路径回收已初始化句柄
    @param [in] listener 监听句柄
    @param [in] status libuv结果
    */
    static void Accepted(uv_stream_t *listener, int status);

    /** 每轮先消费至多128项取消，再消费至多1024项发送，发送同时服从字节及在途窗口预算
    @param [in] handle 发送唤醒
    */
    static void SendPending(uv_async_t *handle);

    /** 只发起关闭，不释放回调借用内存
    @param [in] handle 停止唤醒
    */
    static void StopPending(uv_async_t *handle);

    /** 释放已接纳写请求；立即失败在提交点释放
    @param [in] request 完成请求
    @param [in] status libuv结果
    */
    static void Written(uv_write_t *request, int status);

private:
    // 进程内本库的新线程序号，UINT32_MAX为永久耗尽状态，不回绕复用旧身份
    static std::atomic<uint32_t> m_nextProducerToken;
    // 线程退出无需解引用实例；缓存只持有整数，队列登记由实例自身拥有
#ifdef _MSC_VER
    static __declspec(thread) uint32_t m_producerToken;
#else
    static thread_local uint32_t m_producerToken;
#endif
};

#include "LibuvBase.inl"