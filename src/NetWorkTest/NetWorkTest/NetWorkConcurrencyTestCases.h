#pragma once
#include "NetSender/NetSenderAPI.h"
#include "NetWork/NetWorkAPI.h"
#include <NetWork/NetWorkAPI.h>
#include <NetPeer/NetPeerAPI.h>
#include <NetSender/NetSenderAPI.h>
#include <NetPeer/NetPeerAPI.h>
#include <CTaskThreadManager/CTaskThreadManagerAPI.h>
#include <atomic>
#include <mutex>
#include <map>
#include <vector>

/** 接纳恢复用例的无分配观测状态，不参与正式运输逻辑
*/
struct LibuvAcceptTestState
{
public:
    // 最近服务端真正接纳的连接号
    uint64_t m_peer;
    // 已接纳连接事件总数
    uint32_t m_connected;
    // 已完成关闭事件总数
    uint32_t m_closed;
    // 完整接收字节数，用于跨TCP分包核对内容
    uint64_t m_received;
    // 字节或代次检查结果
    bool m_valid;

public:
    /** 初始化全部观测值，不取得网络资源
    */
    LibuvAcceptTestState();
};

// 隔离监听器的服务消息副本，全部属于测试main执行者
struct NetWorkConcurrencyMessage
{
    // 真实路由代次
    uint64_t m_endpoint;
    // 原帧及独占消息体
    NetInterfaceFrame m_frame;
    // 本用例拥有的消息字节副本，避免在回调外保留失效借用
    std::vector<uint8_t> m_bytes;
};

// 仅模拟业务处理器，不实现TLS、RPC或路由
class NetWorkConcurrencySink : public NetEndpointServiceSink
{
public:
    /** 默认只验证原有回显；保序模式另核对每个生产者序列
    */
    NetWorkConcurrencySink();

    /** 复制借用帧，返回真实接纳结果
    @param [in] endpoint 当前逻辑端点号
    @param [in] frame 本次调用期间借用的完整帧
    @param [in] origin 本次调用期间借用的可信来源
    @return NI_OK已复制测试帧，异常交由测试运行器记录
    */
    virtual int32_t send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetEndpointOrigin& origin);

    /** 此测试没有业务进程可停止
    @param [in] endpoint 当前逻辑端点号
    */
    virtual void close(uint64_t endpoint);

    /** 在main上处理并回送全部已投递请求
    @param [in,out] router 真实Peer路由器
    @return 是否所有帧合法且接纳成功
    */
    bool respond(NetPeerEndpointRouter& router);
public:
    // 当前main拥有的消息
    std::vector<NetWorkConcurrencyMessage> m_messages;
    // 只由用例owner配置／访问，接收顺序直接取自真实路由投递。
    bool m_verifyOrder;
    // 四个生产者各自下一条预期到达序号
    uint32_t m_next[4];
};

// 多生产者及回调的有界观察状态，不持锁进行网络或任务等待
struct NetWorkConcurrencyContext
{
public:
    // main回收，生产者只借用
    NetWorkClientHandle m_client;
    // 测试main系统线程
    uint32_t m_mainThread;
    // 所有回调必须来自本代同一owner
    std::atomic<uint32_t> m_callbackThread;
    // 最近实际握手代次
    std::atomic<uint64_t> m_generation;
    // 并发观察
    std::atomic<uint32_t> m_ready;
    // 本用例观察到的终态记录，用于检查关闭不丢失且不重复
    std::atomic<uint32_t> m_closed;
    // 本用例工作已完成，拥有者仍须收敛运输或回收任务
    std::atomic<uint32_t> m_done;
    // 实际收到的请求终态次数，用于检查一请求一终态
    std::atomic<uint32_t> m_results;
    // 输入、顺序、线程或回调次数不符合用例独立预期时置位
    std::atomic<bool> m_bad;
    // 在ready回调抛受控异常；只在连接前设置，验证协议线程能正常收敛
    bool m_throwReady;
    // 启动前固定，四个生产者各发送带独立递增序号的正文。
    bool m_verifyOrder;
    // 只保护测试的有界编号集合
    std::mutex m_mutex;
    // 同步send真实调用耗时，单位微秒，仅在生产者回收后统计，不作为时延硬阈值。
    std::vector<uint32_t> m_sendMicros;
    // 本用例成功接纳的请求号集合，用于与实际终态逐项对照
    std::vector<uint64_t> m_accepted;
    // 请求号到实际终态次数的映射，用于发现漏回调和重复回调
    std::map<uint64_t, uint32_t> m_terminal;

public:
    /** 清零独占句柄及观察计数
    */
    NetWorkConcurrencyContext();

    /** 记录真实握手、回调线程并核对禁止回调中销毁／重入
    @param [in] context 本测试借用上下文，生命周期覆盖回调；随机填充入口不使用该值
    @param [in] generation 实际会话代次，由被测实现提供
    @param [in] requests 本轮协商后的最大在途请求数
    @param [in] chunk 本轮协商后的最大块字节数
    */
    static void ready(void* context, uint64_t generation, uint32_t requests, uint32_t chunk);

    /** 核对本轮请求的实际终态、正文和回调约束
    @param [in] context 本次用例借用的观察状态，仅在所属执行者回调中访问
    @param [in] generation 当前会话代次，用于核对陈旧回调不会进入新会话
    @param [in] id 本轮实际请求号，必须与已提交请求匹配
    @param [in] product 协议产品号，按本用例独立预期核对
    @param [in] type 业务消息类型，按本用例独立预期核对
    @param [in] status RPC终态状态，不等同于底层发送返回值
    @param [in] bytes 回调或同步调用期间借用的正文；不保存裸指针
    @param [in] size 正文字节数，不能用C字符串长度代替
    */
    static void result(void* context, uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 记录本用例非预期的独立帧回调
    @param [in] context 本次用例借用的观察状态，仅在所属执行者回调中访问
    @param [in] generation 当前会话代次，用于核对陈旧回调不会进入新会话
    @param [in] frame 本次调用期间借用的完整帧，背压时由调用方保留
    @return 非零表示此帧已消费，避免失败观察再次触发回调
    */
    static int32_t frame(void* context, uint64_t generation, const NetInterfaceFrame* frame);

    /** 核对本轮会话的正常终态与协议线程归属
    @param [in] context 本次用例借用的观察状态，仅在所属执行者回调中访问
    @param [in] generation 当前会话代次，用于核对陈旧回调不会进入新会话
    @param [in] reason 本连接或服务的NI关闭原因
    @param [in] detail 底层诊断码，不替代NI关闭原因
    */
    static void closed(void* context, uint64_t generation, int32_t reason, int32_t detail);
};

// 所有生产者也使用CTask，不私建系统线程
class NetWorkConcurrencyProducer : public CTask
{
public:
    /** 借用本轮观察状态至任务退出
    @param [in] context 生命期main保留
    @param [in] producer 本轮生产者编号，保序测试为0至3
    */
    explicit NetWorkConcurrencyProducer(NetWorkConcurrencyContext* context, uint32_t producer = 0);

    /** 独立提交8次send／post混合请求，每三次取消一次；同步交付不持测试锁
    */
    virtual void DoTask();
private:
    // main先回收生产者再释放
    NetWorkConcurrencyContext* m_context;
    // 当前CTask固定的生产者下标0至3，不使用系统线程ID作为协议字段。
    uint32_t m_producer;
};
// 本用例独占的生命期owner，异常时先停止生产者、收敛网络再回收句柄。
class NetWorkConcurrencyOwner
{
public:
    /** 借用比本owner更早创建的观察状态，生产者和回调不得越过其生命期
    @param [in,out] context 本用例观察状态
    */
    explicit NetWorkConcurrencyOwner(NetWorkConcurrencyContext& context);

    /** 在runner执行者回收本用例独占的资源，不操作其他线程ID。
    */
    ~NetWorkConcurrencyOwner();
    // 最多4个独占生产者，每轮结束后清零，失败时由析构回收。
    uint32_t m_producers[4];
private:
    /** 禁止复制独占资源
    @param [in] other 不可复制
    */
    NetWorkConcurrencyOwner(const NetWorkConcurrencyOwner& other);

    /** 禁止赋值独占资源
    @param [in] other 不可复制
    @return 不可调用
    */
    NetWorkConcurrencyOwner& operator=(const NetWorkConcurrencyOwner& other);
    // 外部观察状态仍然存活，供关闭时的最终回调使用。
    NetWorkConcurrencyContext* m_context;
};

// 原生多生产者、异步文件和资源回收测试集合，逐用例独立建立预期并检查实际结果
class NetWorkConcurrencyTestCases
{
public:
    /** 三路真实服务队列共享额度，借用、取消和异常必须正确释放且不破坏期限
    */
    static void serviceQueueOwnership();

    /** 服务接入阶段、失败收敛及回调异常不遗留polling或重复终态
    */
    static void serviceGatewayLifecycle();

    /** 服务依赖出站适配的输入边界、回调异常、代次关闭及输出查询
    */
    static void serviceOutgoingLifecycle();

    /** 验证服务聚合代理的容量、来源、期限和有序重试
    */
    static void serviceProxyLifecycle();

    /** 验证接收回调更新时间不能造成无符号时间差误超时
    */
    static void serviceCallbackClock();

    /** 原生跨线程宿主桥满队列终态、单连接故障及四生产者FIFO
    */
    static void hostBridgeLifecycle();

    /** 临时文件Unicode路径关闭、析构及实际工作线程停止回收
    */
    static void nativeFileCleanup();

    /** 验证网关路径编码、内嵌零及系统字符替代规则
    */
    static void stringConversion();

    /** 验证原生TCP接纳、容量拒绝隔离及关闭后重新服务
    */
    static void libuvAcceptRecovery();

    /** 验证兼容TCP接口未启动、已初始化及析构回收路径
    */
    static void libuvLifecycle();

    /** 验证未连接NetWork句柄关闭、状态查询和回收，无非预期回调
    */
    static void workLifecycle();

    /** 验证Format输出别名、宽文本、空输入及扩容边界
    */
    static void stringFormat();

    /** Windows专属租约正常/失败输入保护、ready和重复释放
    */
    static void processLeaseResources();

    /** 真实异步文件后端32轮任务失败输出、停止门禁和资源收敛
    */
    static void fileFacadeLifecycle();

    /** 核对文件编码、断点前缀、重复块、持久化确认和终态
    */
    static void fileProtocolLifecycle();

    /** 128请求逆序映射、持续背压、取消及跨代次回调隔离
    */
    static void dispatchMappingLifecycle();

    /** Peer统一核心重连、非法重入、关闭异常和64代生命周期
    */
    static void peerFacadeLifecycle();

    /** 同步EOF顺序、四类事件满槽、恢复读取的借用隔离及宿主异常收敛
    */
    static void hostLifecycle();

    /** 两代真实TCP/TLS、八个CTask生产者，停用调用方Poll仍实际完成76个唯一终态。
    */
    static void threadedClients();

    /** 真实原生桥接器遇到部分创建失败/宿主异常时必须关闭和回收，不遗留回调目标
    */
    static void hostCreationFailures();

    /** 创建失败且宿主暂拒销毁时，清理句柄必须可达并可稍后回收
    */
    static void deferredHostCleanup();

    /** 已关闭原始socket的最后字节仍按序交付，不再调用其暂停接口，也不影响另一peer
    */
    static void closedHostTail();

    /** 验证五秒未交付的同步流帧不会迟发，普通post、额度和健康连接仍可用
    */
    static void synchronousFrameExpiry();

    /** 同一提交者反向预留及混合RPC／DATA／ACK时，按实际提交顺序交付
    */
    static void submissionOrder();

    /** 验证未消费输入和交付票据不能跨代，以及关闭期间借用正文保持有效
    */
    static void submissionLifetime();

    /** 四个CTask各自混合send／post，服务端逐生产者验证先发先到
    */
    static void threadedOrder();

    /** 在检查通知后、进入等待前受控投递，验证真实owner通知不会丢失。
    */
    static void ownerWakeup();

    /** 受控检查实际文件Task停止通知；不能让最终回收永久等在空队列。
    */
    static void fileStopWakeup();

    /** 200轮真实原生文件CTask启动／停止，核对停止后拒收、done及线程回落。
    */
    static void fileStopCycles();

    /** 已取消的CLOSE／PUBLISH也回收文件，实际后续SIZE必须返回NOT_FOUND，不靠最终stop兜底。
    */
    static void fileCancelledRelease();

    /** 2万次完整16KiB IPC信封接纳，检查代次、内容、确认顺序及独占缓冲所有权。
    */
    static void ipcInputOwnership();

    /** 测试计时使用单调微秒；Windows显式用QPC，避免v120的chrono时钟精度不足。
    @return 单调微秒；计时器不可用时抛测试异常
    */
    static uint64_t microseconds();

    /** 两个用例共用真实网络夹具
    @param [in] verifyOrder 是否核对实际到达的生产者序列而非取消场景
    */
    static void runThreadedClients(bool verifyOrder);

    /** 暂停连接积压2034事件时保持同连接顺序，并推进未暂停连接
    */
    static void pausedHostFairness();

    /** 8轮预热后64轮真实连接交替正常关闭和回调异常，核对资源回落
    */
    static void lifecycleResources();

    /** 用真实libuv分配器故障验证循环初始化错误不抛出，且实例可安全释放
    */
    static void loopInitializationFailure();
};

struct NetWorkClientOpaque;

// 只调用实际NetWork唤醒入口，main保留client至独占CTask回收，不推进或模拟协议。
class NetWorkWakeProducer : public CTask
{
public:
    /** 借用本轮静止客户端，所有成员先于PostTask完成初始化
    @param [in] client main拥有的内部客户端
    */
    explicit NetWorkWakeProducer(NetWorkClientOpaque* client);

    /** 在独占CTask调用实际wake；原子标志仅协调用例中的临界时序。
    */
    virtual void DoTask();
    // main观察到已开始后才放行condition_variable的原子解锁／等待。
    std::atomic<bool> m_started;
    // 回收前观察唤醒已实际返回。
    std::atomic<bool> m_done;
private:
    // main始终先回收任务，再销毁本实例借用的client。
    NetWorkClientOpaque* m_client;
};

class NetFileIoState;

// 只调用真实文件Task的停止入口，main保留state至本任务回收。
class NetFileStopProducer : public CTask
{
public:
    /** 保存本轮状态强引用，投递前初始化标志
    @param [in] state 本轮独占的文件协调状态
    */
    explicit NetFileStopProducer(const std::shared_ptr<NetFileIoState>& state);

    /** 在独占CTask调用实际StopTask，不模拟文件后端。
    */
    virtual void DoTask();
    // main只读此两项以安排受控竞争；不访问任务内部协议。
    std::atomic<bool> m_started;
    // 本用例工作已完成，拥有者仍须收敛运输或回收任务
    std::atomic<bool> m_done;
private:
    // 保留至任务回收，不包含已打开文件。
    std::shared_ptr<NetFileIoState> m_state;
};

// 仅提供原始事件的测试宿主，不替代网关/TLS实现，不创建系统线程
struct NetWorkAuditHost
{
public:
    // 故障模式只由本用例owner顺序修改
    uint32_t m_mode;
    // 桥接器保存的原始回调目标，销毁后不再调用
    void* m_target;
    // 借用的原始连接回调，模拟宿主销毁后不再调用
    NetHostConnected m_connected;
    // 借用的原始输入回调，只在其上下文仍有效时调用
    NetHostData m_data;
    // 可写事件观察状态；宿主回调只在有效上下文中调用
    NetHostWritable m_writable;
    // 借用的原始终态回调，模拟宿主销毁后不再调用
    NetHostClosed m_closed;
    // 实际资源回收调用数
    uint32_t m_stops;
    // 实际原始宿主destroy调用次数，核对部分创建失败回收
    uint32_t m_destroyed;
    // 故障注入：剩余拒绝销毁次数，不阻塞线程。
    uint32_t m_destroyBusy;
    // 关闭尾部测试的借用桥，先销毁桥再释放本测试宿主。
    void* m_bridge;
    // 原始peer1已释放，继续操作应由假宿主明确拒绝。
    bool m_retired;
    // 独立观察底层操作次数，关闭后不得再增加。
    uint32_t m_operations;
    // 两个连接各自的正文及终态计数，索引0对应peer1
    uint32_t m_inputs[2];
    // 实际关闭通知次数，用于检查唯一终态
    uint32_t m_closes[2];
    // 是否存在未知连接、损坏借用字节或终态后的数据
    bool m_bad;

public:
    /** mode为0正常，1创建后返回错误，2创建后抛异常，3普通操作抛异常
    @param [in] mode 受控失败方式
    */
    explicit NetWorkAuditHost(uint32_t mode);

    /** 返回借用本测试状态的完整宿主表
    @return v1原始宿主表
    */
    NetHostFunctions functions();

    /** 保存回调并在请求的失败点退出，provider指向本测试状态
    @param [in] provider 本用例状态
    @param [in] observer 桥接器借用目标
    @param [in] connected 连接回调
    @param [in] data 数据回调
    @param [in] writable 可写回调
    @param [in] closed 关闭回调
    @param [out] instance 本测试宿主
    @return 0或受控错误；mode2专供测试异常隔离
    */
    static int32_t create(void* provider, void* observer, NetHostConnected connected, NetHostData data, NetHostWritable writable, NetHostClosed closed, void** instance);

    /** 返回固定连接号，不触发事件
    @param [in] instance 本测试状态
    @param [in] address 未使用的数值地址
    @param [in] port 未使用的端口
    @param [out] peer 固定1
    @return 0
    */
    static int32_t connect(void* instance, const char* address, uint32_t port, uint64_t* peer);

    /** 观察停止标志；销毁记录真实调用次数
    @param [in] instance 本测试状态
    @return 1活动/0停止，destroy始终0
    */
    static int32_t poll(void* instance);

    /** 记录模拟宿主停止请求，供外层核对资源收敛
    @param [in] instance 本次用例有效的模拟宿主，不由此入口释放观察状态
    @return 0已接纳停止；受控异常用于验证被测桥接器的隔离
    */
    static int32_t shutdown(void* instance);

    /** 按受控次数拒绝销毁，再记录实际资源回收
    @param [in] instance 本次用例有效的模拟宿主，不由此入口释放观察状态
    @return 0已回收，-42表示本次仍忙；受控异常用于测试隔离
    */
    static int32_t destroy(void* instance);

    /** 不解释正文的完整接纳接口
    @param [in] instance 本测试状态
    @param [in] peer 连接号
    @param [in] bytes 借用正文
    @param [in] size 长度
    @return 0
    */
    static int32_t write(void* instance, uint64_t peer, const uint8_t* bytes, uint32_t size);

    /** 模拟暂停或关闭，不产生嵌套事件
    @param [in] instance 本测试状态
    @param [in] peer 连接号
    @param [in] value 暂停标志或原因
    @return 0
    */
    static int32_t peerOperation(void* instance, uint64_t peer, int32_t value);

    /** 不拥有监听或连接资源
    @param [in] instance 本测试状态
    @return 0
    */
    static uint32_t query(void* instance);

    /** 以下观察者只由用例owner调用，记录两个连接的实际顺序和次数
    @param [in] context 本测试借用上下文，生命周期覆盖回调；随机填充入口不使用该值
    @param [in] peer 当前运输连接号
    */
    static void connected(void* context, uint64_t peer);

    /** 核对输入字节、连接顺序和终态前后事件边界
    @param [in] context 本次用例借用的观察状态，仅在所属执行者回调中访问
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    @param [in] bytes 回调或同步调用期间借用的正文；不保存裸指针
    @param [in] size 正文字节数，不能用C字符串长度代替
    */
    static void data(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size);

    /** 接纳本用例无需观察的可写事件，不修改状态
    @param [in] context 本次用例借用的观察状态，仅在所属执行者回调中访问
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    */
    static void writable(void* context, uint64_t peer);

    /** 核对两个测试连接的正常终态并分别计数
    @param [in] context 本次用例借用的观察状态，仅在所属执行者回调中访问
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    @param [in] reason 本连接或服务的NI关闭原因
    */
    static void closed(void* context, uint64_t peer, int32_t reason);
};

// 只在无活动libuv句柄的隔离初始化用例内替换分配器，不改变系统malloc
class NetWorkAuditAllocation
{
public:
    /** 拒绝本次libuv分配
    @param [in] size 请求字节
    @return nullptr
    */
    static void* allocate(size_t size);

    /** 拒绝调整，不释放既有地址
    @param [in] pointer 借用原地址
    @param [in] size 请求字节
    @return nullptr
    */
    static void* reallocate(void* pointer, size_t size);

    /** 拒绝清零分配
    @param [in] count 数量
    @param [in] size 单项字节
    @return nullptr
    */
    static void* allocateZero(size_t count, size_t size);
};

// 使用真实HostSocket和Work桥，核对最后数据回调可暂停且先于唯一EOF。
class NetWorkTailObserver : public NetHostSocketListener
{
public:
    /** 清空本用例观察状态
    */
    NetWorkTailObserver();

    /** 本用例owner接到连接后恢复本地交付
    @param [in] peer 固定测试连接号1
    */
    virtual void onConnected(uint64_t peer);

    /** 同一owner核对尾部正文并暂停／恢复，不再操作已关闭的原始socket
    @param [in] peer 固定测试连接号1
    @param [in] data 回调内借用的固定字节7
    @param [in] size 必须为1
    */
    virtual void onData(uint64_t peer, const uint8_t* data, uint32_t size);

    /** 本用例不发送正文
    @param [in] peer 忽略的借用连接号
    */
    virtual void onWritable(uint64_t peer);

    /** 同一owner核对终态恰好一次且晚于最后正文
    @param [in] peer 固定测试连接号1
    @param [in] reason 必须为NI_OK
    @param [in] detail 必须为0
    */
    virtual void onClosed(uint64_t peer, int32_t reason, int32_t detail);
    // socket比观察者后创建，关闭及释放均在同一owner完成。
    NetHostSocket* m_socket;
    // 最后数据只计一次。
    uint32_t m_inputs;
    // 关闭终态只计一次。
    uint32_t m_closed;
    // 记录顺序、正文或暂停失败。
    bool m_bad;
};