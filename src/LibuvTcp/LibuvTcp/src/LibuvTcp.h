#pragma once
#include <stdint.h>
#include <atomic>
#include "LibuvTcpMacro.h"
#include "LibuvHostTypes.h"
#include "LibuvTcpPoolConfig.h"

typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_loop_s uv_loop_t;

struct LibuvBase;
struct LibuvPendingSend;
class LibuvLifecycleGate;
struct LibuvPoolSettings;

/** 批量发送的一条借用正文；长度头由库填写，不带TCP指针或业务封装
*/
struct LibuvSendBuffer
{
    // 只借用到提交函数返回；长度为零时允许空指针
    const char* m_data;
    // 正文字节数，可为零
    uint32_t m_size;
};

/** 兼容长度前缀TCP接口，拥有循环和队列；调用方提供loop执行者并负责结束线程后销毁
*/
class LibuvTcpAPI LibuvTcp
{
  public:
    /** 创建原始字节运输，不含协议、加密、压缩或后台线程
    @param [in] providerContext 保留的宿主上下文，本实现忽略
    @param [in] observerContext 四个回调借用的上下文，存活至destroy成功
    @param [in] connected 非空连接建立回调
    @param [in] data 非空借用字节回调
    @param [in] writable 非空本地写完成回调
    @param [in] closed 非空唯一终态回调
    @param [out] instance 不可空，失败清零；只由hostDestroy释放
    @return 0成功，负数未创建；C边界不抛异常
    */
    static int32_t hostCreate(void* providerContext, void* observerContext, LibuvHostConnected connected,
        LibuvHostData data, LibuvHostWritable writable, LibuvHostClosed closed, void** instance);

    /** hostCreate后、hostListen/hostConnect前设置原始字节运输资源参数，同一执行者调用
    @param [in] instance 有效且未启动、未停止的实例
    @param [in] config 写池配置副本，失败不改变已有设置
    @return 0成功，负数参数或状态错误，不分配内存池
    */
    static int32_t hostConfigureWritePool(void* instance, const LibuvChannelWritePoolConfig& config);

    /** hostCreate后、hostListen/hostConnect前设置原始字节运输资源参数，同一执行者调用
    @param [in] instance 有效且未启动、未停止的实例
    @param [in] bufferBytes 每连接读缓冲大小，1至16777216字节
    @return 0成功，负数参数或状态错误，不分配内存池
    */
    static int32_t hostConfigureReadBuffer(void* instance, uint32_t bufferBytes);

    /** 监听数值IPv4/IPv6；本实例仅能启动一次
    @param [in] instance 有效未启动实例，同一执行者调用所有操作
    @param [in] address 非空数值地址，通配监听须明确给出
    @param [in] port 0至65535，0由系统分配
    @param [in] maxPeers 1至256，包含关闭中的连接
    @return 0已监听，负数失败
    */
    static int32_t hostListen(void* instance, const char* address, uint32_t port, uint32_t maxPeers);

    /** 异步连接数值地址；返回成功不表示连接完成
    @param [in] instance 有效未启动实例
    @param [in] address 非空数值IPv4/IPv6
    @param [in] port 1至65535
    @param [out] peer 不可空，成功为不复用连接号，失败0
    @return 0接纳，负数未接纳
    */
    static int32_t hostConnect(void* instance, const char* address, uint32_t port, uint64_t* peer);

    /** 非阻塞驱动；回调内调用被拒绝
    @param [in] instance 有效实例
    @return 1仍有资源，0已经收敛，负数非法状态
    */
    static int32_t hostPoll(void* instance);

    /** 复制完整字节后异步写，默认每次至多32KiB、每连接在途64KiB/64次，可在启动前配置
    @param [in] instance 有效实例
    @param [in] peer 已连接且未关闭的连接号
    @param [in] data size非零时不可空，只借用到返回
    @param [in] size 字节数，0无操作
    @return 0完整接纳，1背压未接纳且无副作用，负数未接纳失败
    */
    static int32_t hostWrite(void* instance, uint64_t peer, const uint8_t* data, uint32_t size);

    /** 暂停或恢复输入，回调中可调用
    @param [in] instance 有效实例
    @param [in] peer 已连接的连接号
    @param [in] paused 仅0或1
    @return 0成功，负数失败
    */
    static int32_t hostPauseRead(void* instance, uint64_t peer, int32_t paused);

    /** 请求关闭，允许在回调中调用；不存在的peer无操作
    @param [in] instance 有效实例
    @param [in] peer 连接号
    @param [in] reason 交给唯一终态的原因
    @return 0已请求，负数非法状态
    */
    static int32_t hostClose(void* instance, uint64_t peer, int32_t reason);

    /** 回收所属连接，回调中仅标记停止由外层poll完成
    @param [in] instance 有效实例
    @return 0已请求或已停止，负数失败
    */
    static int32_t hostShutdown(void* instance);

    /** 同一执行者停止并释放，成功后绝无后续回调
    @param [in] instance 有效实例或null；成功后不得重复使用，poll/回调内拒绝且不释放
    @return 0已经释放，负数未释放
    */
    static int32_t hostDestroy(void* instance);

    /** 查询实际监听端口
    @param [in] instance 实例或null
    @return 监听端口，未监听或null为0
    */
    static uint32_t hostLocalPort(void* instance);

    /** 查询包括关闭中连接的拥有数
    @param [in] instance 实例或null
    @return 当前数量，null为0
    */
    static uint32_t hostPeerCount(void* instance);

    /** 创建无连接实例；init/loop属于同一串行生命周期
     */
    LibuvTcp();

    /** 兜底收敛资源；派生宿主须先stop并等待loop结束再销毁派生状态
    禁止在本实例的回调/loop执行者中析构，不能与新进入的方法调用并发销毁
    */
    virtual ~LibuvTcp();

  public:
    /** 初始化前设置发送队列，只复制参数；不与任何其他实例操作并发
    @param [in] config 借用至返回；失败保留上次有效配置，运行期间拒绝修改
    @return 0成功，UV_EINVAL参数非法，UV_EBUSY实例已经初始化；最终组合检查在init执行
    */
    int32_t configureSendQueue(const LibuvQueuePoolConfig& config);

    /** 初始化前设置独立关闭队列，只复制参数；不与任何其他实例操作并发
    @param [in] config 借用至返回；失败保留上次有效配置，运行期间拒绝修改
    @return 0成功，UV_EINVAL参数非法，UV_EBUSY实例已经初始化；最终组合检查在init执行
    */
    int32_t configureCloseQueue(const LibuvQueuePoolConfig& config);

    /** 初始化前设置跨线程连接索引，只复制参数；不与任何其他实例操作并发
    @param [in] config 借用至返回；失败保留上次有效配置，运行期间拒绝修改
    @return 0成功，UV_EINVAL参数非法，UV_EBUSY实例已经初始化；最终组合检查在init执行
    */
    int32_t configureConnectionPool(const LibuvConnectionPoolConfig& config);

    /** 初始化前设置本库发送预算和缓存，只复制参数；不与任何其他实例操作并发
    @param [in] config 借用至返回；失败保留上次有效配置，运行期间拒绝修改
    @return 0成功，UV_EINVAL参数非法，UV_EBUSY实例已经初始化；最终组合检查在init执行
    */
    int32_t configureSendPool(const LibuvSendPoolConfig& config);

    /** 初始化前设置读缓冲池，只复制参数；不与任何其他实例操作并发
    @param [in] config 借用至返回；失败保留上次有效配置，运行期间拒绝修改
    @return 0成功，UV_EINVAL参数非法，UV_EBUSY实例已经初始化；最终组合检查在init执行
    */
    int32_t configureReadPool(const LibuvReadPoolConfig& config);

    /** 接纳异步IPv4连接，失败完整回滚，可在loop结束后重试
    @param [in] ip 非空数值IPv4地址
    @param [in] port 1至65535
    @return 接纳成功，不代表已经连通；活跃实例拒绝重复init
    */
    bool initClient(const char *ip, int32_t port);

    /** 初始化IPv4监听，失败完整回滚
    @param [in] port 0至65535，0由系统分配
    @param [in] backlog 正监听预算
    @return 是否监听成功
    */
    bool initServer(int32_t port, int32_t backlog = 128);

    /** 串行初始化指定IPv4监听地址；本机测试使用127.0.0.1，不向其他网卡暴露监听
    @param [in] address 数值IPv4地址，不能为空
    @param [in] port 0至65535，0自动分配端口
    @param [in] backlog 正数监听队列长度
    @return 是否完成初始化，不能与loop或其他生命周期操作并发
    */
    bool initServer(const char* address, int32_t port, int32_t backlog = 128);

    /** 唯一消费执行者，所有uv操作及业务回调在此执行，返回前收敛全部自有资源
    未初始化、已结束或重入调用无操作；宿主拥有本方法执行线程并负责等待
    */
    void loop();

    /** 线程安全地请求停止，重复调用安全，不提前释放句柄或清空上下文
     */
    void stop();

    /** 申请发送数组，必须与send在本库配对转移所有权
    @param [in] length 完整数组字节数，包括TCP指针、4字节长度及正文，须为正
    @return 独占数组，非正数为空，分配失败可抛bad_alloc
    */
    char *memoryForSend(int32_t length);

    /** 释放仍由调用方拥有的发送数组；与本库分配配对，避免跨DLL/CRT释放
    无需初始化连接，可多线程释放互不相同的数组；已成功提交的数组不能再释放
    @param [in] buffer memoryForSend返回且尚未转移所有权的数组，允许空
    */
    void releaseSendMemory(const char* buffer);

    /** 线程安全普通提交，始终接管数组；等同 trySend 的两个重试选项为 true
    @param [in] buffer 本库分配的完整数组，可空；依次为uv_tcp_t*、int32正文长度和正文
    调用方保证容量覆盖头及非负正文，提交后不得再修改/释放；线数据仍为长度加正文
    本库队列及libuv在途共享配置预算，默认64MiB；空间不足持续重试，前4096次后每次请求休眠1毫秒
    不能从本实例循环回调等待空间；停止、连接失效、非法调用或内存失败会结束等待
    @return true 已接纳，false 其他错误且数组已回收；不以临时空间不足返回false，不是对端确认
    */
    bool send(const char *buffer);

    /** 尝试转移完整数组，失败时可重试或用releaseSendMemory释放，成功后不得再访问数组
    @param [in] buffer memoryForSend 分配的完整指针、长度和正文，容量由调用方保证
    @param [in] retryContention CAS竞争时是否持续重试，默认false
    @param [in] waitForSpace 空间不足时是否持续等待，默认false；前4096次后每次休眠1毫秒
    @return 0 已接纳；UV_EAGAIN 竞争；UV_ENOBUFS 空间不足且未选择等待；其他负数未接纳
    等待可因停止、连接失效或系统错误结束；本实例loop回调或未运行循环不得等待容量
    请求本身超过实例总预算为UV_EINVAL，不能等待永远无法满足的容量
    */
    int32_t trySend(const char* buffer, bool retryContention = false, bool waitForSpace = false);

    /** 线程安全地复制并提交一条正文，内部填写长度头；不需要调用方另分配发送数组
    @param [in] target 本实例有效连接借用
    @param [in] data 正文，仅借用到返回；size为零时允许空
    @param [in] size 正文字节数，请求及写请求开销不能超过64MiB总预算
    @return true单条已接纳，false参数、连接、停止或资源错误；不是对端确认
    竞争与容量等待规则和数组版send一致，返回后可修改或释放原正文
    */
    bool send(uv_tcp_t* target, const char* data, uint32_t size);

    /** 尝试复制提交一条正文，失败不发布数据且不改变调用方所有权
    @param [in] target 本实例有效连接借用
    @param [in] data 借用正文，非零长度时不可空
    @param [in] size 正文字节数，超过实例预算为UV_EINVAL
    @param [in] retryContention 是否持续重试CAS竞争
    @param [in] waitForSpace 是否等待容量，4096次后每次请求休眠1毫秒
    @return 0已接纳；其他错误及循环内等待限制和数组版trySend相同
    */
    int32_t trySend(uv_tcp_t* target, const char* data, uint32_t size,
        bool retryContention = false, bool waitForSpace = false);

    /** 线程安全地复制并接纳一批长度帧；顺序和线格式与依次send相同，不接管输入
    @param [in] target 本实例仍有效的连接借用
    @param [in] buffers 非空正文描述数组，数组和正文借用到返回
    @param [in] count 1至4096条；所有正文及每条4字节长度头合计至多1MiB
    @return true整批接纳；false停止、失效、参数或资源错误，不存在部分接纳
    */
    bool sendBatch(uv_tcp_t* target, const LibuvSendBuffer* buffers, uint32_t count);

    /** 线程安全批量复制提交；可选择竞争和容量等待，不改变输入所有权
    @param [in] target 本实例仍有效的连接借用
    @param [in] buffers 借用正文数组，返回后可修改或释放
    @param [in] count 1至4096条，总线上长度至多1MiB
    @param [in] retryContention 是否持续重试CAS竞争
    @param [in] waitForSpace 是否等待实际容量，4096次后每次请求休眠1毫秒
    @return 0整批接纳；错误码与trySend一致，失败无部分接纳，输入始终由调用方拥有
    */
    int32_t trySendBatch(uv_tcp_t* target, const LibuvSendBuffer* buffers, uint32_t count,
        bool retryContention = false, bool waitForSpace = false);

    /** 线程安全地投递完整数组，由唯一loop执行者处理；成功仅承诺本地队列接纳
    @param [in] buffer 本库分配的TCP指针、int32长度和正文数组，可空；无论结果始终接管
    @return true已投递，false参数、停止、失效或资源错误且数组已释放，不是网络完成确认
    与send共用有界队列并主动唤醒loop，不新增线程；竞争持续重试，容量等待4096次后每次休眠1毫秒
    本实例loop回调或尚未运行的loop不得等待自身容量；与send混用仍保持每个生产者的提交顺序
    */
    bool post(const char* buffer);

    /** 尝试投递完整数组；成功转移所有权，失败仍归调用方，可重试或releaseSendMemory
    @param [in] buffer memoryForSend分配的完整数组，容量由调用方保证
    @param [in] retryContention 是否持续重试CAS竞争，默认false
    @param [in] waitForSpace 是否等待容量，默认false；4096次后每次休眠1毫秒
    @return 0本地接纳，UV_EAGAIN竞争，UV_ENOBUFS容量不足，其他负数未接纳；错误码与trySend一致
    可多线程调用不同数组；等待可因停止、失效或错误结束，禁止等待自身loop释放容量
    */
    int32_t tryPost(const char* buffer, bool retryContention = false, bool waitForSpace = false);

    /** 线程安全地复制投递一条正文，内部填写长度头，返回后原正文可立即复用
    @param [in] target 本实例当前有效连接回调给出的借用指针，不能跨断开保留
    @param [in] data 借用到返回，成功或失败都归调用方；size为零时可空
    @param [in] size 正文字节数，请求及写请求开销不得超过配置总预算，默认64MiB
    @return true本地接纳，false参数、停止、失效或资源错误；不保证对端接收或网络写完成
    竞争、容量等待、owner限制与数组版post相同；与send共用顺序和预算
    */
    bool post(uv_tcp_t* target, const char* data, uint32_t size);

    /** 线程安全地尝试复制投递一条正文，失败不发布消息、不转移原正文所有权
    @param [in] target 本实例当前有效连接借用
    @param [in] data 借用正文，非零长度时不可空
    @param [in] size 正文字节数，超过实例总预算为UV_EINVAL
    @param [in] retryContention 是否持续重试CAS竞争，默认false
    @param [in] waitForSpace 是否等待容量，默认false；4096次后每次休眠1毫秒
    @return 0本地接纳，其他错误码及owner等待限制与数组版tryPost相同
    */
    int32_t tryPost(uv_tcp_t* target, const char* data, uint32_t size,
        bool retryContention = false, bool waitForSpace = false);

    /** 线程安全地复制投递一批正文，不接管输入；整批共用一项队列，失败无部分接纳
    @param [in] target 本实例当前有效连接借用
    @param [in] buffers 非空描述数组，数组及正文借用到返回
    @param [in] count 1至4096条，所有正文及每条4字节长度头合计至多1MiB
    @return true整批本地接纳，false停止、失效、参数或资源错误；等待规则同post
    */
    bool postBatch(uv_tcp_t* target, const LibuvSendBuffer* buffers, uint32_t count);

    /** 线程安全地尝试复制投递整批正文，返回后可复用原数据，失败无部分发布
    @param [in] target 本实例当前有效连接借用
    @param [in] buffers 借用正文数组，不接管任何输入
    @param [in] count 1至4096条，所有长度头及正文合计至多1MiB
    @param [in] retryContention 是否持续重试CAS竞争，默认false
    @param [in] waitForSpace 是否等待容量，默认false；4096次后每次休眠1毫秒
    @return 0整批本地接纳，其他错误码及owner等待限制同tryPost
    */
    int32_t tryPostBatch(uv_tcp_t* target, const LibuvSendBuffer* buffers, uint32_t count,
        bool retryContention = false, bool waitForSpace = false);

    /** 借用循环，仅在init成功至loop返回间有效
    @return 循环或空；外部添加的句柄须自行在stop前关闭，不得并发驱动循环
    */
    uv_loop_t *loopPtr();

    /** 查询当前初始化角色，停止收敛期间仍保留角色
    @return 客户端为true，未初始化/已结束/服务端为false
    */
    bool isClient();

    /** 线程安全提交幂等关闭，由loop优先消费，拒绝非当前连接的指针
    关闭是取消，不是刷新确认；可取消尚未处理的数据，也不等待网络写窗口空闲
    循环内直接请求关闭；跨线程等待取消队列容量，4096次后每次休眠1毫秒，停止会结束等待
    @param [in] tcp 当前借用TCP，空/已关闭无操作；不能跨连接代次保留旧指针
    */
    void close(uv_tcp_t *tcp);

    /** 在loop执行者借用接收数据；异常被封装边界收敛为停止
    @param [in] sender 当前连接
    @param [in] buffer 借用至回调返回，禁止保存裸地址
    @param [in] length 实际正字节数
    */
    virtual void receive(uv_tcp_t *sender, const char *buffer, int32_t length);

    /** 服务端收到连接，TCP借用到断开，不负责delete
    @param [in] client 当前TCP
    */
    virtual void uvClientConnected(uv_tcp_t *client);

    /** 服务端断开通知，句柄内存仍有效至回调返回，不得保存为后续连接使用
    @param [in] client 已请求关闭的当前TCP
    */
    virtual void uvClientDisconnected(uv_tcp_t *client);

    /** 客户端连接完成通知
    @param [in] server 当前TCP
    */
    virtual void uvServerConnected(uv_tcp_t *server);

    /** 异步连接失败通知，停止后loop收敛
     */
    virtual void uvServerNotFind();

    /** 连接失败清理回调，在失败通知之前执行
     */
    virtual void uvServerNotFindClear();

    /** 客户端断开通知，触发时机由读错误或TCP保活决定，不承诺固定10秒
    @param [in] server 已请求关闭的当前TCP
    */
    virtual void uvServerDisconnected(uv_tcp_t *server);

    /** 断开清理回调，借用TCP尚未释放
    @param [in] tcp 本次断开连接
    */
    virtual void uvDisconnectedClear(uv_tcp_t *tcp);

  public:
    // 仅兼容旧诊断借用，业务不得修改，也不得在其他线程直接解引用
    LibuvBase *m_libuv;

  private:
    // 内部循环在关闭uv句柄前封闭提交入口，不对外暴露回收协议
    friend struct LibuvBase;

    /** 循环owner拒绝新借用并等待原提交调用结束，不从持有借用的send/stop中调用
    */
    void closeSubmissions();

    /** send/post共用的数组提交实现，避免两套校验及所有权逻辑产生分歧
    @param [in] buffer 本库完整发送数组，成功接管，失败不接管
    @param [in] retryContention 是否持续重试竞争
    @param [in] waitForSpace 是否等待容量
    @return 0接纳，负数未接纳；同trySend错误码
    */
    int32_t trySubmitBuffer(const char* buffer, bool retryContention, bool waitForSpace);

    /** send/post共用的正文复制实现，输入只借用到返回
    @param [in] target 当前连接借用
    @param [in] data 正文，size为零时可空
    @param [in] size 正文字节数
    @param [in] retryContention 是否持续重试竞争
    @param [in] waitForSpace 是否等待容量
    @return 0接纳，负数未接纳；输入所有权不变
    */
    int32_t trySubmitData(uv_tcp_t* target, const char* data, uint32_t size,
        bool retryContention, bool waitForSpace);

    /** send/post共用的批量复制实现，保持整批原子接纳与生产者顺序
    @param [in] target 当前连接借用
    @param [in] buffers 借用正文描述数组
    @param [in] count 消息数，1至4096
    @param [in] retryContention 是否持续重试竞争
    @param [in] waitForSpace 是否等待容量
    @return 0整批接纳，负数未接纳；输入所有权不变
    */
    int32_t trySubmitBatch(uv_tcp_t* target, const LibuvSendBuffer* buffers, uint32_t count,
        bool retryContention, bool waitForSpace);

    /** 在共同生命周期和容量边界内提交已构造操作；成功转移数组，失败不转移
    @param [in,out] pending 本次独占操作，尚未取得连接代次
    @param [in] retryContention 是否持续重试竞争
    @param [in] waitForSpace 是否等待容量
    @return 0完整接纳，其他为错误码
    */
    int32_t trySubmit(LibuvPendingSend& pending, bool retryContention, bool waitForSpace);

    /** 收敛上下文后按真实类型销毁，不在uv回调内调用
    @param [in] state 独占上下文，可空
    */
    static void DestroyState(LibuvBase *state);

    /** 禁止复制循环和缓冲所有权
    @param [in] other 原实例
    */
    LibuvTcp(const LibuvTcp &other);

    /** 禁止赋值循环所有权
    @param [in] other 原实例
    @return 不可调用
    */
    LibuvTcp &operator=(const LibuvTcp &other);

  private:
    // 外层拥有的非独占借用计数，覆盖上下文及uv_async句柄的回收边界
    LibuvLifecycleGate* m_lifecycle;
    // 初始化前复制的配置，运行期间不可变，供下次初始化继续使用
    LibuvPoolSettings* m_settings;
    // 唯一loop的存活标志，false的release写是执行者最后一次访问外层对象
    std::atomic<bool> m_running;
};