#pragma once
#include <stdint.h>
#include "LibuvTcpMacro.h"
#include "LibuvHostTypes.h"

typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_loop_s uv_loop_t;

struct LibuvBase;
class LibuvLifecycleGate;

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

    /** 复制完整字节后异步写，每次至多32KiB，每连接在途64KiB/64次
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

    /** 线程安全提交，调用后始终接管数组；停止或目标失效时释放而不解引用旧TCP
    @param [in] buffer 本库分配的完整数组，可空；依次为uv_tcp_t*、int32正文长度和正文
    调用方保证容量覆盖头及非负正文，提交后不得再修改/释放；线数据仍为长度加正文
    */
    void send(const char *buffer);

    /** 借用循环，仅在init成功至loop返回间有效
    @return 循环或空；外部添加的句柄须自行在stop前关闭，不得并发驱动循环
    */
    uv_loop_t *loopPtr();

    /** 查询当前初始化角色，停止收敛期间仍保留角色
    @return 客户端为true，未初始化/已结束/服务端为false
    */
    bool isClient();

    /** 线程安全提交幂等关闭，由loop消费，拒绝非当前连接的指针
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
    // 独占内部同步原语，避免把平台类型或可分配的CRT锁暴露到公共头
    LibuvLifecycleGate* m_lifecycle;
    // 受m_lifecycle保护，同一实例只有一个loop
    bool m_running;
};