#pragma once
#include <mutex>
#include <map>
#include <stdint.h>
#include <atomic>
#include "uv.h"

/** 兼容接口的生命周期短锁及完成通知；锁定、停止和析构不再依赖CRT首次分配
*/
class LibuvLifecycleGate
{
public:
    /** 初始化原语，部分失败由valid报告
    */
    LibuvLifecycleGate();

    /** 使用者和等待者全部退出后释放原语
    */
    ~LibuvLifecycleGate();

    /** 查询是否可用
    @return 两个原语均初始化成功为true
    */
    bool valid() const;

    /** 取得独占短锁，调用前须valid
    */
    void lock();

    /** 释放本线程持有的短锁
    */
    void unlock();

    /** 持锁时原子解锁等待，返回时重新持锁
    @return false表示系统等待失败，由调用者继续检查完成状态
    */
    bool wait();

    /** 状态在锁内改变后唤醒所有生命周期等待者
    */
    void notifyAll();
private:
    /** 禁止复制同步原语
    @param [in] other 原实例
    */
    LibuvLifecycleGate(const LibuvLifecycleGate& other);

    /** 禁止复制活动生命周期
    @param [in] other 原实例
    @return 不可调用
    */
    LibuvLifecycleGate& operator=(const LibuvLifecycleGate& other);
private:
#ifdef _WIN32
    // 与已支持的Windows Vista及以上平台一致，不使用CRT惰性锁上下文
    SRWLOCK m_mutex;
    // 生命周期状态变化的原生条件变量，等待前必须持有m_mutex
    CONDITION_VARIABLE m_changed;
#else
    // Linux、Android及Apple平台的原生同步原语
    pthread_mutex_t m_mutex;
    // 生命周期状态变化的原生条件变量，等待前必须持有m_mutex
    pthread_cond_t m_changed;
#endif
    // 分步初始化所有权，失败构造也能安全析构
    bool m_mutexReady;
    // 条件变量已初始化，失败构造时禁止等待或销毁它
    bool m_conditionReady;
};

// 排队时保存连接代次，消费时不凭可能已经释放/复用的TCP裸地址发送。
struct LibuvPendingSend
{
public:
    // 拥有的发送数组，消费或取消后在本库释放；关闭操作为空
    char *m_buffer;
    // 仅作登记表查询键的TCP借用地址，发送前须同时核对代次
    uv_tcp_t *m_target;
    // 提交时记录的连接代次，防止旧地址复用导致串线
    uint64_t m_generation;
    // true表示关闭目标连接，false表示提交数据
    bool m_close;

public:
    /** 初始化空操作，不拥有TCP句柄
     */
    LibuvPendingSend();
};

class LibuvTcp;
typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_loop_s uv_loop_t;
typedef struct uv_async_s uv_async_t;
typedef struct uv_connect_s uv_connect_t;
template <class QElmType> class LockFreeQueue;

/** 兼容接口的循环资源所有者；循环执行者关闭句柄，提交线程仅访问受锁保护的登记和队列
*/
struct LibuvBase
{
public:
    // 借用的外层宿主，存活至全部uv回调及排空结束
    LibuvTcp *m_libuvTcp;
    // 拥有的私有循环，成功初始化后才能运行和关闭
    uv_loop_t *m_loop;
    // 发送队列唤醒句柄，允许多生产者提交后唤醒唯一循环
    uv_async_t *m_asyncHandle;
    // 拥有的待发送或关闭操作队列，停止时消费并回收剩余数组
    LockFreeQueue<LibuvPendingSend> *m_queue;
    // 停止专用唤醒句柄，避免关闭请求依赖新的队列分配
    uv_async_t *m_asyncCloseHandle;
    // 派生上下文类型标记，true为客户端，false为服务端
    bool m_isClient;

    // 保护连接登记及提交，uv_run和业务回调不持锁
    LibuvLifecycleGate m_mutex;
    // 地址仅作查找键，排队同时保存代次
    std::map<uv_tcp_t *, uint64_t> m_connections;
    // 允许提交线程请求停止，句柄只由循环线程关闭
    std::atomic<bool> m_stopping;
    // 下次登记的单调连接代次，耗尽后停止接纳而不复用
    uint64_t m_nextConnection;
    // 循环初始化成功，排空后才能调用uv_loop_close
    bool m_loopInitialized;
    // 循环执行者已开始幂等关闭，禁止再次提交读写
    bool m_closing;

public:
    /** 创建空的循环、队列及唤醒存储，实际uv初始化由initialize完成
    */
    LibuvBase();

    /** 所有uv回调结束后释放构造阶段内存
     */
    ~LibuvBase();

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

    /** 每轮最多128项代次绑定的发送/关闭操作
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
};