#include "LibuvHostState.h"
#include "LibuvTcp.h"
#include "LibuvBase.h"
#include "LibuvClient.h"
#include "LibuvServer.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <memory>
#include <cstring>
#include <climits>
#include <cstdio>

#ifdef _WIN32
#pragma comment(lib, "Dbghelp.lib")
#endif

LibuvTcp::LibuvTcp() :
m_libuv(nullptr),
m_lifecycle(new LibuvLifecycleGate()),
m_running(false)
{

}

LibuvTcp::~LibuvTcp()
{
    if (!m_lifecycle->valid())
    {
        delete m_lifecycle;
        return;
    }
    std::unique_lock<LibuvLifecycleGate> lock(*m_lifecycle);
    if (m_libuv != nullptr && m_running)
    {
        {
            std::lock_guard<LibuvLifecycleGate> submit(m_libuv->m_mutex);
            if (!m_libuv->m_stopping.exchange(true))
            {
                uv_async_send(m_libuv->m_asyncCloseHandle);
            }
        }
        // 宿主仍须先停止/等待自己的派生对象；严禁回调中delete this。
        while (m_running)
        {
            if (!m_lifecycle->wait())
            {
                // 仅系统条件等待失败时退到有界间隔复查，不能提前释放仍在运行的上下文。
                lock.unlock();
                uv_sleep(1);
                lock.lock();
            }
        }
    }
    LibuvBase *state = m_libuv;
    m_libuv = nullptr;
    lock.unlock();
    if (state != nullptr)
    {
        state->m_libuvTcp = nullptr;
    }
    DestroyState(state);
    delete m_lifecycle;
}

void LibuvTcp::DestroyState(LibuvBase *state)
{
    if (state == nullptr)
    {
        return;
    }
    state->drain();
    if (state->m_loopInitialized)
    {
        // 外部通过loopPtr添加的句柄仍须由外部先关闭，不能释放仍忙碌的循环内存。
        std::fputs("LibuvTcp loop still busy: external handles must be closed before teardown\n", stderr);
        return;
    }
    if (state->m_isClient)
    {
        delete static_cast<LibuvClient *>(state);
    }
    else
    {
        delete static_cast<LibuvServer *>(state);
    }
}

bool LibuvTcp::initClient(const char *ip, int32_t port)
{
    if (!m_lifecycle->valid())
    {
        return false;
    }
    sockaddr_in address = {};
    if (ip == nullptr || port < 1 || port > 65535 || uv_ip4_addr(ip, port, &address) != 0)
    {
        return false;
    }
    std::lock_guard<LibuvLifecycleGate> lock(*m_lifecycle);
    if (m_libuv != nullptr || m_running)
    {
        return false;
    }
    std::unique_ptr<LibuvClient> state(new LibuvClient);
    int32_t result = state->initialize(this);
    if (result == 0)
    {
        state->m_connect->data = state.get();
        result = uv_tcp_connect(
            state->m_connect, state->m_dest, reinterpret_cast<const sockaddr *>(&address), &LibuvBase::Connected);
    }
    if (result != 0)
    {
        state->m_libuvTcp = nullptr;
        DestroyState(state.release());
        return false;
    }
    m_libuv = state.release();
    return true;
}

bool LibuvTcp::initServer(int32_t port, int32_t backlog)
{
    if (!m_lifecycle->valid())
    {
        return false;
    }
    if (port < 0 || port > 65535 || backlog < 1)
    {
        return false;
    }
    std::lock_guard<LibuvLifecycleGate> lock(*m_lifecycle);
    if (m_libuv != nullptr || m_running)
    {
        return false;
    }
    std::unique_ptr<LibuvServer> state(new LibuvServer);
    int32_t result = state->initialize(this);
    sockaddr_in address = {};
    if (result == 0)
    {
        result = uv_ip4_addr("0.0.0.0", port, &address);
    }
    if (result == 0)
    {
        result = uv_tcp_bind(state->m_server, reinterpret_cast<const sockaddr *>(&address), 0);
    }
    if (result == 0)
    {
        result = uv_listen(reinterpret_cast<uv_stream_t *>(state->m_server), backlog, &LibuvBase::Accepted);
    }
    if (result != 0)
    {
        state->m_libuvTcp = nullptr;
        DestroyState(state.release());
        return false;
    }
    m_libuv = state.release();
    return true;
}

void LibuvTcp::loop()
{
    if (!m_lifecycle->valid())
    {
        return;
    }
    LibuvBase *state = nullptr;
    {
        std::lock_guard<LibuvLifecycleGate> lock(*m_lifecycle);
        if (m_libuv == nullptr || m_running)
        {
            return;
        }
        state = m_libuv;
        m_running = true;
    }
    uv_run(state->m_loop, UV_RUN_DEFAULT);
    // 停止先禁止提交，再收敛写完成/连接取消/close回调，最后才释放队列与句柄内存。
    state->drain();
    {
        std::lock_guard<LibuvLifecycleGate> lock(*m_lifecycle);
        m_libuv = nullptr;
        DestroyState(state);
        m_running = false;
        // 通知也在生命周期锁内完成，析构等待者不能抢先销毁条件变量。
        m_lifecycle->notifyAll();
    }
}

void LibuvTcp::stop()
{
    if (!m_lifecycle->valid())
    {
        return;
    }
    std::lock_guard<LibuvLifecycleGate> lock(*m_lifecycle);
    if (m_libuv == nullptr)
    {
        return;
    }
    std::lock_guard<LibuvLifecycleGate> submit(m_libuv->m_mutex);
    if (!m_libuv->m_stopping.exchange(true))
    {
        uv_async_send(m_libuv->m_asyncCloseHandle);
    }
}

char *LibuvTcp::memoryForSend(int32_t length)
{
    return length <= 0 ? nullptr : new char[static_cast<size_t>(length)];
}

void LibuvTcp::send(const char *buffer)
{
    std::unique_ptr<char[]> bytes(const_cast<char *>(buffer));
    if (buffer == nullptr || !m_lifecycle->valid())
    {
        return;
    }
    LibuvPendingSend pending;
    std::memcpy(&pending.m_target, buffer, sizeof(pending.m_target));
    int32_t length = 0;
    std::memcpy(&length, buffer + sizeof(uv_tcp_t *), sizeof(length));
    if (length < 0 || length > INT_MAX - 4 || pending.m_target == nullptr)
    {
        return;
    }
    std::lock_guard<LibuvLifecycleGate> lock(*m_lifecycle);
    if (m_libuv == nullptr)
    {
        return;
    }
    std::lock_guard<LibuvLifecycleGate> submit(m_libuv->m_mutex);
    const auto entry = m_libuv->m_connections.find(pending.m_target);
    if (m_libuv->m_stopping.load() || entry == m_libuv->m_connections.end())
    {
        return;
    }
    pending.m_generation = entry->second;
    pending.m_buffer = bytes.get();
    try
    {
        m_libuv->m_queue->push(pending);
        bytes.release();
        uv_async_send(m_libuv->m_asyncHandle);
    }
    catch (...)
    {
        m_libuv->m_stopping.store(true);
        uv_async_send(m_libuv->m_asyncCloseHandle);
    }
}

uv_loop_t *LibuvTcp::loopPtr()
{
    if (!m_lifecycle->valid())
    {
        return nullptr;
    }
    std::lock_guard<LibuvLifecycleGate> lock(*m_lifecycle);
    return m_libuv == nullptr ? nullptr : m_libuv->m_loop;
}

bool LibuvTcp::isClient()
{
    if (!m_lifecycle->valid())
    {
        return false;
    }
    std::lock_guard<LibuvLifecycleGate> lock(*m_lifecycle);
    return m_libuv != nullptr && m_libuv->m_isClient;
}

void LibuvTcp::close(uv_tcp_t *tcp)
{
    if (!m_lifecycle->valid())
    {
        return;
    }
    std::lock_guard<LibuvLifecycleGate> lock(*m_lifecycle);
    if (m_libuv == nullptr || tcp == nullptr)
    {
        return;
    }
    std::lock_guard<LibuvLifecycleGate> submit(m_libuv->m_mutex);
    const auto entry = m_libuv->m_connections.find(tcp);
    if (m_libuv->m_stopping.load() || entry == m_libuv->m_connections.end())
    {
        return;
    }
    LibuvPendingSend pending;
    pending.m_target = tcp;
    pending.m_generation = entry->second;
    pending.m_close = true;
    try
    {
        m_libuv->m_queue->push(pending);
        uv_async_send(m_libuv->m_asyncHandle);
    }
    catch (...)
    {
        m_libuv->m_stopping.store(true);
        uv_async_send(m_libuv->m_asyncCloseHandle);
    }
}

void LibuvTcp::receive(uv_tcp_t* sender, const char* buffer, int32_t length)
{
	printf("sender = %p, buffer = %p, length = %d\n", sender, buffer, length);
}

void LibuvTcp::uvClientConnected(uv_tcp_t* client)
{
	printf("client = %p\n", client);
}

void LibuvTcp::uvClientDisconnected(uv_tcp_t* client)
{
	printf("client disconnect = %p\n", client);
}

void LibuvTcp::uvServerConnected(uv_tcp_t* server)
{
	printf("server = %p\n", server);
}

void LibuvTcp::uvServerNotFind()
{
	printf("server not find\n");
}

void LibuvTcp::uvServerNotFindClear()
{

}

void LibuvTcp::uvServerDisconnected(uv_tcp_t* server)
{
	printf("server disconnect = %p\n", server);
}

void LibuvTcp::uvDisconnectedClear(uv_tcp_t* tcp)
{

}

//class Client : public LibuvTcp
//{
//public:
//	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length)
//	{
//		int x = 3;
//	}
//
//	virtual void clientConnected(uv_tcp_t* client)
//	{
//
//	}
//
//	virtual void serverConnected(uv_tcp_t* server)
//	{
//		int x = 3;
//		//send(server, "123456", 6, 0);
//	}
//};
//
//class Server : public LibuvTcp
//{
//public:
//	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length)
//	{
//		char* s = buffer + 8;
//		int x = 3;
//		//send(client, "111", 3, 0);
//	}
//
//	virtual void clientConnected(uv_tcp_t* client)
//	{
//		int x = 3;
//	}
//
//	virtual void serverConnected(uv_tcp_t* server)
//	{
//
//	}
//};
//
//int main()
//{
//	Server server;
//	server.initServer(7000);
//	server.loop();
//	Client client;
//	client.initClient("127.0.0.1", 7000);
//	client.loop();
//	getchar();
//	return 0;
//}

// LIBUV_TCP_AUDIT_SELF_TEST_BEGIN
// #include "LibuvChannel.h"
// #include "CTaskThreadManager/CTaskThreadManagerAPI.h"
// #include "Semaphore/SemaphoreAPI.h"
// #include <atomic>
// #include <vector>
//
// // 仅供本段注释main临时启用的观察者，不参与正式库编译。
// class LibuvTcpAuditProbe : public LibuvTcp
// {
//   public:
//     /** 初始化无连接测试观察者
//     */
//     LibuvTcpAuditProbe();
//
//     /** 接收原始线字节，按测试配置抛异常
//     @param [in] sender 本次连接
//     @param [in] buffer 借用字节
//     @param [in] length 字节数
//     */
//     virtual void receive(uv_tcp_t *sender, const char *buffer, int32_t length);
//
//     /** 记录客户端连接
//     @param [in] client 本次TCP
//     */
//     virtual void uvClientConnected(uv_tcp_t *client);
//
//     /** 记录服务器连接
//     @param [in] server 本次TCP
//     */
//     virtual void uvServerConnected(uv_tcp_t *server);
//
//     /** 记录断开
//     @param [in] client 本次TCP
//     */
//     virtual void uvClientDisconnected(uv_tcp_t *client);
//
//     /** 记录断开
//     @param [in] server 本次TCP
//     */
//     virtual void uvServerDisconnected(uv_tcp_t *server);
//
//     /** 记录异步连接失败
//     */
//     virtual void uvServerNotFind();
//
//     /** 连接失败清理不添加行为
//     */
//     virtual void uvServerNotFindClear();
//
//     /** 断开清理不添加行为
//     @param [in] tcp 本次TCP
//     */
//     virtual void uvDisconnectedClear(uv_tcp_t *tcp);
//
//   public:
//     // 以下原子值用于主测试执行者观察，字节数组只在join后读取
//     std::atomic<uv_tcp_t *> m_peer;
//     // 已接收字节数，由测试主线程原子观察
//     std::atomic<uint32_t> m_size;
//     // 已收到的断开回调数，由测试主线程原子观察
//     std::atomic<uint32_t> m_closed;
//     // 异步连接失败次数，由测试主线程原子观察
//     std::atomic<uint32_t> m_notFound;
//     // 仅接收执行者追加的字节副本，主线程等待执行者结束后读取
//     std::vector<uint8_t> m_bytes;
//     // 启动前写入的场景配置
//     bool m_throwRead;
//     // 是否在连接回调等待测试屏障，只在启动前配置
//     bool m_gateConnect;
//     // 通知主测试线程连接回调已进入的屏障
//     Semaphore m_connected;
//     // 允许阻塞中的连接回调继续的测试屏障
//     Semaphore m_release;
// };
//
// // 测试的循环/生产者执行者全部复用CTask，不自行创建线程。
// class LibuvTcpAuditTask : public CTask
// {
//   public:
//     /** 固定本次借用目标和任务种类
//     @param [in] owner 借用到WaitForEnd
//     @param [in] producer true发送，false运行loop
//     */
//     LibuvTcpAuditTask(LibuvTcpAuditProbe *owner, bool producer);
//
//     /** 执行有界场景
//     */
//     virtual void DoTask();
//
//     /** 停止本次目标并释放测试屏障
//     */
//     virtual void StopTask();
//
//   public:
//     // 借用的被测实例，存活至任务结束并WaitForEnd完成
//     LibuvTcpAuditProbe *m_owner;
//     // true为发送任务，false为独占loop任务
//     bool m_producer;
//     // 任务已结束的原子观测标志
//     std::atomic<bool> m_done;
//     // 任务断言是否失败的原子观测标志
//     std::atomic<bool> m_failed;
//     // 生产者完成前半段时通知主测试线程
//     Semaphore m_halfway;
//     // 主测试线程完成故障设置后允许生产者继续
//     Semaphore m_continue;
// };
//
// // 观察新原始TCP接口在析构回调内再次请求shutdown的收敛。
// class LibuvChannelAuditProbe : public LibuvChannelListener
// {
//   public:
//     /** 初始化观察者
//     */
//     LibuvChannelAuditProbe();
//
//     /** 记录连接
//     @param [in] peer 本代次
//     */
//     virtual void onConnected(uint64_t peer);
//
//     /** 接收不添加行为
//     @param [in] peer 本代次
//     @param [in] data 借用数据
//     @param [in] size 字节数
//     */
//     virtual void onData(uint64_t peer, const uint8_t *data, uint32_t size);
//
//     /** 写完成不添加行为
//     @param [in] peer 本代次
//     */
//     virtual void onWritable(uint64_t peer);
//
//     /** 记录唯一终态并请求shutdown
//     @param [in] peer 本代次
//     @param [in] reason 原因
//     */
//     virtual void onClosed(uint64_t peer, int32_t reason);
//
//   public:
//     // 借用到终态回调返回的Channel，在回调里测试重复shutdown
//     LibuvChannel *m_channel;
//     // 最近已通知的连接号，由单个poll执行者读写
//     uint64_t m_peer;
//     // 已收到的断开回调数，由测试主线程原子观察
//     uint32_t m_closed;
// };
// #include "CSystem/CSystemAPI.h"
// #include <stdexcept>
// #include <cstdlib>
// #include <chrono>
// #include <functional>
//
// static std::atomic<int64_t> g_libuvAuditArrays(0);
// // 分配器观察不能取得CRT/STL锁或再次分配，固定原子槽避免测试自己引入锁递归。
// static std::atomic<void *> g_libuvAuditBuffers[65536];
// static std::atomic<bool> g_libuvAuditTracking(false);
//
// /** 只在独立测试EXE中统计new[]所有权，不替换产品分配器
// @param [in] size 字节数
// @return 可释放的独占内存
// */
// void *operator new[](size_t size)
// {
//     void *memory = std::malloc(size == 0 ? 1 : size);
//     if (memory == nullptr)
//     {
//         throw std::bad_alloc();
//     }
//     return memory;
// }
//
// /** 统计独立测试EXE的数组释放
// @param [in] memory 本模块分配的数组
// */
// void operator delete[](void *memory) throw()
// {
//     if (memory != nullptr)
//     {
//         if (g_libuvAuditTracking.load() && g_libuvAuditArrays.load() != 0)
//         {
//             const size_t start = (reinterpret_cast<uintptr_t>(memory) >> 4) & 65535;
//             for (size_t index = 0; index < 65536; ++index)
//             {
//                 std::atomic<void *> &slot = g_libuvAuditBuffers[(start + index) & 65535];
//                 void *value = slot.load();
//                 if (value == nullptr)
//                 {
//                     break;
//                 }
//                 if (value == memory &&
//                     slot.compare_exchange_strong(value, reinterpret_cast<void *>(static_cast<uintptr_t>(1))))
//                 {
//                     --g_libuvAuditArrays;
//                     break;
//                 }
//             }
//         }
//         std::free(memory);
//     }
// }
//
// /** 不抛异常的测试数组分配
// @param [in] size 字节数
// @param [in] tag 不抛异常标记
// @return 独占内存或空
// */
// void *operator new[](size_t size, const std::nothrow_t &tag) throw()
// {
//     (void)tag;
//     try
//     {
//         return operator new[](size);
//     }
//     catch (...)
//     {
//         return nullptr;
//     }
// }
//
// /** 不抛异常分配对应的失败释放
// @param [in] memory 独占数组
// @param [in] tag 不抛异常标记
// */
// void operator delete[](void *memory, const std::nothrow_t &tag) throw()
// {
//     (void)tag;
//     operator delete[](memory);
// }
//
// /** 核对独立预期，失败使测试EXE非零退出
// @param [in] condition 必须成立的事实
// @param [in] message 失败原因
// */
// static void AuditRequire(bool condition, const char *message)
// {
//     if (!condition)
//     {
//         throw std::runtime_error(message);
//     }
// }
//
// /** 用固定8秒截止观察异步完成，不延长预算
// @param [in] predicate 完成事实
// */
// static void AuditWait(const std::function<bool()> &predicate)
// {
//     const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
//     while (!predicate())
//     {
//         AuditRequire(std::chrono::steady_clock::now() < deadline, "asynchronous deadline exceeded");
//         CSystem::Sleep(1);
//     }
// }
//
// /** 构造旧协议固定字节向量，包含TCP指针及4字节正文长度
// @param [in] owner 创建数组的库实例
// @param [in] tcp 借用目标，允许过期值用于反例
// @param [in] size 正文字节数
// @return 转移给send的独占数组
// */
// static char *AuditPacket(LibuvTcp &owner, uv_tcp_t *tcp, uint32_t size)
// {
//     char *bytes = owner.memoryForSend(static_cast<int32_t>(sizeof(tcp) + 4 + size));
//     const size_t start = (reinterpret_cast<uintptr_t>(bytes) >> 4) & 65535;
//     bool inserted = false;
//     for (size_t index = 0; index < 65536 && !inserted; ++index)
//     {
//         std::atomic<void *> &slot = g_libuvAuditBuffers[(start + index) & 65535];
//         void *value = slot.load();
//         if (value == nullptr || value == reinterpret_cast<void *>(static_cast<uintptr_t>(1)))
//         {
//             inserted = slot.compare_exchange_strong(value, bytes);
//         }
//     }
//     AuditRequire(inserted, "test allocation tracking budget exceeded");
//     ++g_libuvAuditArrays;
//     const int32_t length = static_cast<int32_t>(size);
//     std::memcpy(bytes, &tcp, sizeof(tcp));
//     std::memcpy(bytes + sizeof(tcp), &length, 4);
//     for (uint32_t index = 0; index < size; ++index)
//     {
//         bytes[sizeof(tcp) + 4 + index] = static_cast<char>(index * 31 + 7);
//     }
//     return bytes;
// }
//
// /** 读取本批0端口监听的实际本机端口
// @param [in] owner 当前服务端实例
// @return 分配端口
// */
// static uint32_t AuditPort(LibuvTcp &owner)
// {
//     sockaddr_in address = {};
//     int size = sizeof(address);
//     uv_tcp_t *tcp = static_cast<LibuvServer *>(owner.m_libuv)->m_server;
//     AuditRequire(uv_tcp_getsockname(tcp, reinterpret_cast<sockaddr *>(&address), &size) == 0, "getsockname failed");
//     return ntohs(address.sin_port);
// }
//
// LibuvTcpAuditProbe::LibuvTcpAuditProbe() :
// m_peer(nullptr),
// m_size(0),
// m_closed(0),
// m_notFound(0),
// m_throwRead(false),
// m_gateConnect(false)
// {
//
// }
//
// void LibuvTcpAuditProbe::receive(uv_tcp_t *sender, const char *buffer, int32_t length)
// {
//     (void)sender;
//     if (m_throwRead)
//     {
//         throw std::runtime_error("controlled receive exception");
//     }
//     m_bytes.insert(m_bytes.end(), buffer, buffer + length);
//     m_size.store(static_cast<uint32_t>(m_bytes.size()));
// }
//
// void LibuvTcpAuditProbe::uvClientConnected(uv_tcp_t *client)
// {
//     m_peer.store(client);
//     m_connected.signal();
//     if (m_gateConnect)
//     {
//         m_release.wait();
//     }
// }
//
// void LibuvTcpAuditProbe::uvServerConnected(uv_tcp_t *server)
// {
//     m_peer.store(server);
//     m_connected.signal();
// }
//
// void LibuvTcpAuditProbe::uvClientDisconnected(uv_tcp_t *client)
// {
//     (void)client;
//     ++m_closed;
// }
// void LibuvTcpAuditProbe::uvServerDisconnected(uv_tcp_t *server)
// {
//     (void)server;
//     ++m_closed;
// }
// void LibuvTcpAuditProbe::uvServerNotFind()
// {
//     ++m_notFound;
// }
// void LibuvTcpAuditProbe::uvServerNotFindClear()
// {
//
// }
// void LibuvTcpAuditProbe::uvDisconnectedClear(uv_tcp_t *tcp)
// {
//     (void)tcp;
// }
//
// LibuvTcpAuditTask::LibuvTcpAuditTask(LibuvTcpAuditProbe *owner, bool producer) :
// m_owner(owner),
// m_producer(producer),
// m_done(false),
// m_failed(false)
// {
//
// }
//
// void LibuvTcpAuditTask::DoTask()
// {
//     try
//     {
//         if (!m_producer)
//         {
//             m_owner->loop();
//         }
//         else
//         {
//             for (uint32_t index = 0; index < 1024; ++index)
//             {
//                 m_owner->send(AuditPacket(*m_owner, m_owner->m_peer.load(), 64));
//                 if (index == 127)
//                 {
//                     m_halfway.signal();
//                     m_continue.wait();
//                 }
//             }
//         }
//     }
//     catch (...)
//     {
//         m_failed.store(true);
//         m_owner->stop();
//     }
//     m_done.store(true);
// }
//
// void LibuvTcpAuditTask::StopTask()
// {
//     m_owner->stop();
//     m_owner->m_release.signal();
//     m_continue.signal();
// }
//
// /** 创建并提交本批任务，借用目标活到最终WaitForEnd
// @param [in] task 本批任务
// @return 独占线程ID
// */
// static uint32_t AuditStart(const std::shared_ptr<LibuvTcpAuditTask> &task)
// {
//     const uint32_t thread = CTaskThreadManager::Instance().Init();
//     auto executor = CTaskThreadManager::Instance().GetThreadInterface(thread);
//     AuditRequire(executor != nullptr && executor->TryPostTask(task), "task submission failed");
//     return thread;
// }
//
// LibuvChannelAuditProbe::LibuvChannelAuditProbe() :
// m_channel(nullptr),
// m_peer(0),
// m_closed(0)
// {
//
// }
// void LibuvChannelAuditProbe::onConnected(uint64_t peer)
// {
//     m_peer = peer;
// }
// void LibuvChannelAuditProbe::onData(uint64_t peer, const uint8_t *data, uint32_t size)
// {
//     (void)peer;
//     (void)data;
//     (void)size;
// }
// void LibuvChannelAuditProbe::onWritable(uint64_t peer)
// {
//     (void)peer;
// }
// void LibuvChannelAuditProbe::onClosed(uint64_t peer, int32_t reason)
// {
//     (void)peer;
//     (void)reason;
//     ++m_closed;
//     if (m_channel != nullptr)
//     {
//         m_channel->shutdown();
//     }
// }
//
// /** 真实本机双向旧协议或受控关闭/异常/多生产者场景
// @param [in] scenario 3双向，4重复关闭和旧队列，5发送停止并发，6回调异常
// */
// static void AuditRoundTrip(uint32_t scenario)
// {
//     LibuvTcpAuditProbe server;
//     LibuvTcpAuditProbe client;
//     server.m_gateConnect = scenario == 4;
//     server.m_throwRead = scenario == 6;
//     AuditRequire(server.initServer(0), "server init failed");
//     AuditRequire(client.initClient("127.0.0.1", AuditPort(server)), "client init failed");
//     auto serverTask = std::make_shared<LibuvTcpAuditTask>(&server, false);
//     auto clientTask = std::make_shared<LibuvTcpAuditTask>(&client, false);
//     std::vector<uint32_t> threads;
//     std::vector<std::shared_ptr<LibuvTcpAuditTask>> producers;
//     threads.push_back(AuditStart(serverTask));
//     threads.push_back(AuditStart(clientTask));
//     try
//     {
//         AuditWait([&]() { return server.m_peer.load() != nullptr && client.m_peer.load() != nullptr; });
//         if (scenario == 4)
//         {
//             uv_tcp_t *old = server.m_peer.load();
//             server.close(old);
//             server.close(old);
//             for (uint32_t index = 0; index < 1024; ++index)
//             {
//                 server.send(AuditPacket(server, old, 64));
//             }
//             server.m_release.signal();
//             AuditWait([&]() { return client.m_closed.load() == 1; });
//             server.send(AuditPacket(server, old, 64));
//         }
//         else if (scenario == 5)
//         {
//             for (uint32_t index = 0; index < 4; ++index)
//             {
//                 auto task = std::make_shared<LibuvTcpAuditTask>(&client, true);
//                 producers.push_back(task);
//                 threads.push_back(AuditStart(task));
//             }
//             for (const auto &task : producers)
//             {
//                 AuditRequire(task->m_halfway.wait(8000), "producer barrier timeout");
//             }
//             client.stop();
//             for (const auto &task : producers)
//             {
//                 task->m_continue.signal();
//             }
//             for (const auto &task : producers)
//             {
//                 AuditWait([&]() { return task->m_done.load(); });
//             }
//         }
//         else
//         {
//             client.send(AuditPacket(client, client.m_peer.load(), 65536));
//             if (scenario == 6)
//             {
//                 AuditWait([&]() { return serverTask->m_done.load(); });
//             }
//             else
//             {
//                 server.send(AuditPacket(server, server.m_peer.load(), 65536));
//                 AuditWait([&]() { return server.m_size.load() == 65540 && client.m_size.load() == 65540; });
//             }
//         }
//     }
//     catch (...)
//     {
//         server.m_release.signal();
//         for (const auto &task : producers)
//         {
//             task->m_continue.signal();
//         }
//         client.stop();
//         server.stop();
//         for (uint32_t thread : threads)
//         {
//             CTaskThreadManager::Instance().WaitForEnd(thread);
//         }
//         throw;
//     }
//     client.stop();
//     server.stop();
//     for (uint32_t thread : threads)
//     {
//         CTaskThreadManager::Instance().WaitForEnd(thread);
//     }
//     AuditRequire(!serverTask->m_failed.load() && !clientTask->m_failed.load(), "exception escaped libuv callback");
//     AuditRequire(server.m_libuv == nullptr && client.m_libuv == nullptr, "context retained after loop");
//     if (scenario == 3)
//     {
//         std::vector<uint8_t> expected(65540);
//         const uint32_t length = 65536;
//         std::memcpy(expected.data(), &length, 4);
//         for (uint32_t index = 0; index < length; ++index)
//         {
//             expected[4 + index] = static_cast<uint8_t>(index * 31 + 7);
//         }
//         AuditRequire(server.m_bytes == expected && client.m_bytes == expected, "wire bytes changed or incomplete");
//     }
// }
//
// /** 执行一个稳定编号的有界回归
// @param [in] id 1至10
// */
// static void AuditCase(uint32_t id)
// {
//     const int64_t arrays = g_libuvAuditArrays.load();
//     if (id == 1)
//     {
//         LibuvTcp owner;
//         AuditRequire(owner.initServer(0), "init failed");
//         owner.stop();
//         owner.stop();
//         owner.loop();
//         owner.send(AuditPacket(owner, nullptr, 16));
//         AuditRequire(owner.loopPtr() == nullptr && !owner.isClient(), "stopped getters unsafe");
//         owner.loop();
//     }
//     else if (id == 2)
//     {
//         LibuvTcp listener;
//         AuditRequire(listener.initServer(0), "listen failed");
//         const uint32_t port = AuditPort(listener);
//         for (uint32_t index = 0; index < 64; ++index)
//         {
//             LibuvTcp owner;
//             AuditRequire(!owner.initClient("not an IP", 25), "invalid address accepted");
//             AuditRequire(!owner.initServer(static_cast<int32_t>(port)), "occupied port accepted");
//             AuditRequire(owner.initServer(0), "failure poisoned next init");
//             owner.stop();
//             owner.loop();
//         }
//         listener.stop();
//         listener.loop();
//     }
//     else if (id >= 3 && id <= 6)
//     {
//         AuditRoundTrip(id);
//     }
//     else if (id == 7)
//     {
//         LibuvTcp reserved;
//         AuditRequire(reserved.initServer(0), "reserve failed");
//         const uint32_t port = AuditPort(reserved);
//         reserved.stop();
//         reserved.loop();
//         for (uint32_t index = 0; index < 32; ++index)
//         {
//             LibuvTcpAuditProbe owner;
//             AuditRequire(owner.initClient("127.0.0.1", static_cast<int32_t>(port)), "connect not accepted");
//             owner.loop();
//             AuditRequire(
//                 owner.m_notFound.load() == 1 && owner.m_peer.load() == nullptr, "connect failure callback wrong");
//         }
//     }
//     else if (id == 8)
//     {
//         LibuvTcp owner;
//         AuditRequire(owner.initServer(0), "init failed");
//         LibuvBase *state = owner.m_libuv;
//         uv_tcp_t *listener = static_cast<LibuvServer *>(state)->m_server;
// #ifdef LIBUV_TCP_BASELINE_TEST
//         extern void onAsyncCallback(uv_async_t * handle);
// #else
//         AuditRequire(state->registerTcp(listener), "failure fixture registration failed");
// #endif
//         owner.send(AuditPacket(owner, listener, 64));
// #ifdef LIBUV_TCP_BASELINE_TEST
//         onAsyncCallback(state->m_asyncHandle);
// #else
//         LibuvBase::SendPending(state->m_asyncHandle);
// #endif
//         std::printf("ARRAYS immediate before=%lld after=%lld\n", static_cast<long long>(arrays),
//             static_cast<long long>(g_libuvAuditArrays.load()));
//         AuditRequire(g_libuvAuditArrays.load() == arrays, "uv_write immediate failure leaked send buffer");
//         owner.stop();
//         owner.loop();
//     }
//     else if (id == 9)
//     {
//         LibuvChannelAuditProbe serverEvents;
//         LibuvChannelAuditProbe clientEvents;
//         {
//             LibuvChannel server;
//             LibuvChannel client;
//             clientEvents.m_channel = &client;
//             AuditRequire(server.listen("127.0.0.1", 0, &serverEvents) == 0, "raw server failed");
//             uint64_t peer = 0;
//             AuditRequire(
//                 client.connect("127.0.0.1", server.localPort(), &clientEvents, peer) == 0, "raw client failed");
//             AuditWait(
//                 [&]()
//                 {
//                     server.poll();
//                     client.poll();
//                     return serverEvents.m_peer != 0 && clientEvents.m_peer != 0;
//                 });
//             const uint8_t byte = 23;
//             for (uint32_t index = 0; index < 64; ++index)
//             {
//                 AuditRequire(client.write(peer, &byte, 1) == 0, "raw write failed");
//             }
//             // client析构时的onClosed合法地再次请求shutdown，不能重入uv_run。
//         }
//         AuditRequire(clientEvents.m_closed == 1 && serverEvents.m_closed == 1, "destructor close not unique");
//     }
//     else if (id == 10)
//     {
//         // 先隔离Winsock/CRT首次初始化的进程级常驻句柄，再比较稳态256轮。
//         {
//             LibuvTcp warmup;
//             AuditRequire(warmup.initServer(0), "warmup failed");
//         }
// #ifdef _WIN32
//         DWORD beforeHandles = 0;
//         AuditRequire(GetProcessHandleCount(GetCurrentProcess(), &beforeHandles) != 0, "handle count failed");
// #endif
//         for (uint32_t index = 0; index < 256; ++index)
//         {
//             LibuvTcp owner;
//             AuditRequire(owner.initServer(0), "repeated init failed");
//             // 不调用loop的已初始化实例，析构必须排空关闭回调而不是泄漏。
//         }
// #ifdef _WIN32
//         DWORD afterHandles = 0;
//         GetProcessHandleCount(GetCurrentProcess(), &afterHandles);
//         std::printf("HANDLES before=%lu after=%lu\n", beforeHandles, afterHandles);
//         AuditRequire(
//             GetProcessHandleCount(GetCurrentProcess(), &afterHandles) != 0 && afterHandles <= beforeHandles + 4,
//             "destructor leaked kernel handles across repeated initialization");
// #endif
//     }
//     std::printf("ARRAYS final before=%lld after=%lld\n", static_cast<long long>(arrays),
//         static_cast<long long>(g_libuvAuditArrays.load()));
//     AuditRequire(g_libuvAuditArrays.load() == arrays, "owned arrays leaked after case");
// }
//
// /** 临时启用的独立测试入口；0/无参运行全部，1至10单独运行
// @param [in] argc 参数数
// @param [in] argv 稳定测试编号
// @return 0通过、1失败、2非法参数
// */
// int main(int argc, char **argv)
// {
//     std::printf("LIBUV_VERSION header=%u.%u.%u linked=%s tcp=%llu/%llu loop=%llu/%llu\n", UV_VERSION_MAJOR,
//         UV_VERSION_MINOR, UV_VERSION_PATCH, uv_version_string(), static_cast<unsigned long long>(sizeof(uv_tcp_t)),
//         static_cast<unsigned long long>(uv_handle_size(UV_TCP)), static_cast<unsigned long long>(sizeof(uv_loop_t)),
//         static_cast<unsigned long long>(uv_loop_size()));
//     uint32_t selected = 0;
//     if (argc > 2)
//     {
//         return 2;
//     }
//     if (argc == 2)
//     {
//         char *end = nullptr;
//         const long value = std::strtol(argv[1], &end, 10);
//         if (end == argv[1] || *end != '\0' || value < 0 || value > 10)
//         {
//             return 2;
//         }
//         selected = static_cast<uint32_t>(value);
//     }
//     g_libuvAuditTracking.store(true);
//     try
//     {
//         for (uint32_t id = 1; id <= 10; ++id)
//         {
//             if (selected != 0 && id != selected)
//             {
//                 continue;
//             }
//             AuditCase(id);
//             std::printf("LIBUV_AUDIT_PASS id=%u\n", id);
//             std::fflush(stdout);
//         }
//         g_libuvAuditTracking.store(false);
//         std::puts("Libuv legacy lifecycle regression PASS");
//         return 0;
//     }
//     catch (const std::exception &error)
//     {
//         std::fprintf(stderr, "LIBUV_AUDIT_FAIL: %s\n", error.what());
//     }
//     catch (...)
//     {
//         std::fputs("LIBUV_AUDIT_FAIL: unexpected exception\n", stderr);
//     }
//     g_libuvAuditTracking.store(false);
//     return 1;
// }
// LIBUV_TCP_AUDIT_SELF_TEST_END

// LIBUV_ACCEPT_UNIT_BEGIN
// /** 接纳恢复用例的无分配观测状态，不参与正式运输逻辑
// */
// struct LibuvAcceptTestState
// {
// public:
//     // 最近服务端真正接纳的连接号
//     uint64_t m_peer;
//     // 已接纳连接事件总数
//     uint32_t m_connected;
//     // 已完成关闭事件总数
//     uint32_t m_closed;
//     // 完整接收字节数，用于跨TCP分包核对内容
//     uint64_t m_received;
//     // 字节或代次检查结果
//     bool m_valid;
//
// public:
//     /** 初始化全部观测值，不取得网络资源
//     */
//     LibuvAcceptTestState();
// };
// #include "LibuvChannelModels.h"
// #include <CSystem/CSystemAPI.h>
// #include <chrono>
// #include <cstdio>
// #if defined(_WIN32) && defined(_DEBUG) && !defined(LIBUV_ACCEPT_NO_HEAP)
// #include <crtdbg.h>
// // 仅独立main在唯一poll线程安装，业务回调不分配测试观测节点。
// static uint32_t s_acceptFaultMode = 0;
// static uint32_t s_acceptRejected = 0;
// static bool s_acceptPeerAllocated = false;
// /** 只拒绝封装层连接对象或其紧随的map登记，第三方网络事件仍正常推进
// @param [in] operation CRT操作，其余参数沿用CRT原型
// @return 是否允许当前分配
// */
// static int __cdecl AcceptMemoryHook(int operation, void*, size_t size, int block, long, const unsigned char*, int)
// {
//     if (operation != _HOOK_ALLOC || block != _NORMAL_BLOCK)
//     {
//         return 1;
//     }
//     if (s_acceptFaultMode == 2 && s_acceptPeerAllocated)
//     {
//         s_acceptPeerAllocated = false;
//         ++s_acceptRejected;
//         return 0;
//     }
//     if (size == sizeof(LibuvChannelPeer))
//     {
//         if (s_acceptFaultMode == 1)
//         {
//             ++s_acceptRejected;
//             return 0;
//         }
//         s_acceptPeerAllocated = s_acceptFaultMode == 2;
//     }
//     return 1;
// }
// #endif
//
// LibuvAcceptTestState::LibuvAcceptTestState() :
// m_peer(0),
// m_connected(0),
// m_closed(0),
// m_received(0),
// m_valid(true)
// {
// }
//
// /** 记录真正由服务端接纳的连接号
// @param [in] context 测试状态
// @param [in] peer 连接号
// */
// static void AcceptConnected(void* context, uint64_t peer)
// {
//     LibuvAcceptTestState* state = static_cast<LibuvAcceptTestState*>(context);
//     state->m_peer = peer;
//     ++state->m_connected;
// }
//
// /** 按累计字节位置核对内容，不依赖TCP分包方式
// @param [in] context 测试状态
// @param [in] peer 收到数据的连接
// @param [in] bytes 借用输入
// @param [in] size 实际长度
// */
// static void AcceptData(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size)
// {
//     LibuvAcceptTestState* state = static_cast<LibuvAcceptTestState*>(context);
//     state->m_valid = state->m_valid && peer == state->m_peer && size != 0 && bytes != nullptr;
//     for (uint32_t index = 0; index < size; ++index)
//     {
//         state->m_valid = state->m_valid && bytes[index] == static_cast<uint8_t>((state->m_received + index) % 251);
//     }
//     state->m_received += size;
// }
//
// /** 本用例不使用写完成推动后续工作
// @param [in] context 测试状态
// @param [in] peer 连接号
// */
// static void AcceptWritable(void* context, uint64_t peer)
// {
//     (void)context;
//     (void)peer;
// }
//
// /** 统计终态且不在回调中销毁宿主
// @param [in] context 测试状态
// @param [in] peer 连接号
// @param [in] reason 终态原因
// */
// static void AcceptClosed(void* context, uint64_t peer, int32_t reason)
// {
//     (void)peer;
//     (void)reason;
//     ++static_cast<LibuvAcceptTestState*>(context)->m_closed;
// }
//
// /** 推进本用例的四个独占实例，不改变调用线程
// @param [in] hosts 服务端和三个客户端，可含空实例
// */
// static void AcceptTick(void** hosts)
// {
//     for (uint32_t index = 0; index < 4; ++index)
//     {
//         if (hosts[index] != nullptr)
//         {
//             LibuvTcp::hostPoll(hosts[index]);
//         }
//     }
//     CSystem::sleepPrecise(1);
// }
//
// /** 在临时端口验证接纳恢复及满员拒绝不影响已有连接
// @param [in] mode 0正常，1拒绝连接对象，2拒绝登记节点；分配注入仅独立Debug main启用
// @return 是否通过并已关闭全部实例
// */
// static bool LibuvAcceptRound(uint32_t mode)
// {
//     void* hosts[4] = {};
//     LibuvAcceptTestState states[4];
//     bool valid = true;
//     uint32_t checks = 0;
// #define ACCEPT_CHECK(value) { ++checks; if (!(value)) { std::printf("Libuv accept mode=%u line=%d FAIL\n", mode, __LINE__); valid = false; break; } }
//     do
//     {
//         for (uint32_t index = 0; index < 4; ++index)
//         {
//             if (LibuvTcp::hostCreate(nullptr, &states[index], AcceptConnected, AcceptData, AcceptWritable, AcceptClosed, &hosts[index]) != 0)
//             {
//                 valid = false;
//                 break;
//             }
//         }
//         ACCEPT_CHECK(valid);
//         ACCEPT_CHECK(LibuvTcp::hostListen(hosts[0], "127.0.0.1", 0, 1) == 0);
//         const uint32_t port = LibuvTcp::hostLocalPort(hosts[0]);
//         uint64_t peer = 0;
//         ACCEPT_CHECK(LibuvTcp::hostConnect(hosts[1], "127.0.0.1", port, &peer) == 0);
// #if defined(_WIN32) && defined(_DEBUG) && !defined(LIBUV_ACCEPT_NO_HEAP)
//         if (mode != 0)
//         {
//             s_acceptFaultMode = mode;
//             s_acceptRejected = 0;
//             s_acceptPeerAllocated = false;
//             for (uint32_t tick = 0; tick < 32; ++tick)
//             {
//                 LibuvTcp::hostPoll(hosts[1]);
//                 const _CRT_ALLOC_HOOK old = _CrtSetAllocHook(AcceptMemoryHook);
//                 LibuvTcp::hostPoll(hosts[0]);
//                 _CrtSetAllocHook(old);
//                 CSystem::sleepPrecise(1);
//             }
//             s_acceptFaultMode = 0;
//             ACCEPT_CHECK(s_acceptRejected != 0 && states[0].m_connected == 0 && states[1].m_connected == 1);
//         }
// #endif
//         // 不创建第二个连接刺激事件；只在资源恢复后poll，第一条连接必须能自行接纳。
//         auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
//         while ((states[0].m_connected != 1 || states[1].m_connected != 1) && std::chrono::steady_clock::now() < deadline)
//         {
//             AcceptTick(hosts);
//         }
//         ACCEPT_CHECK(states[0].m_connected == 1 && states[1].m_connected == 1);
//         uint8_t bytes[16384];
//         for (uint32_t index = 0; index < sizeof(bytes); ++index)
//         {
//             bytes[index] = static_cast<uint8_t>(index % 251);
//         }
//         ACCEPT_CHECK(LibuvTcp::hostWrite(hosts[1], states[1].m_peer, bytes, sizeof(bytes)) == 0);
//         deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
//         while (states[0].m_received != sizeof(bytes) && std::chrono::steady_clock::now() < deadline)
//         {
//             AcceptTick(hosts);
//         }
//         ACCEPT_CHECK(states[0].m_valid && states[0].m_received == sizeof(bytes));
//         const uint64_t firstPeer = states[0].m_peer;
//         ACCEPT_CHECK(LibuvTcp::hostConnect(hosts[2], "127.0.0.1", port, &peer) == 0);
//         deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
//         while (states[2].m_closed == 0 && std::chrono::steady_clock::now() < deadline)
//         {
//             AcceptTick(hosts);
//         }
//         ACCEPT_CHECK(states[2].m_closed == 1 && states[0].m_closed == 0 && LibuvTcp::hostPeerCount(hosts[0]) == 1);
//         ACCEPT_CHECK(LibuvTcp::hostClose(hosts[0], firstPeer, 0) == 0);
//         deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
//         while ((states[0].m_closed != 1 || states[1].m_closed != 1) && std::chrono::steady_clock::now() < deadline)
//         {
//             AcceptTick(hosts);
//         }
//         ACCEPT_CHECK(states[0].m_closed == 1 && states[1].m_closed == 1 && LibuvTcp::hostPeerCount(hosts[0]) == 0);
//         ACCEPT_CHECK(LibuvTcp::hostConnect(hosts[3], "127.0.0.1", port, &peer) == 0);
//         deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
//         while (states[0].m_connected != 2 && std::chrono::steady_clock::now() < deadline)
//         {
//             AcceptTick(hosts);
//         }
//         ACCEPT_CHECK(states[0].m_connected == 2 && states[0].m_peer > firstPeer);
//     } while (false);
//     for (uint32_t index = 4; index != 0; --index)
//     {
//         if (hosts[index - 1] != nullptr && LibuvTcp::hostDestroy(hosts[index - 1]) != 0)
//         {
//             valid = false;
//         }
//     }
//     valid = valid && states[0].m_closed == states[0].m_connected && states[0].m_valid;
//     std::printf("Libuv accept mode=%u checks=%u received=%llu %s\n", mode, checks,
//         static_cast<unsigned long long>(states[0].m_received), valid ? "PASS" : "FAIL");
//     return valid;
// #undef ACCEPT_CHECK
// }
//
// /** 执行真实TCP恢复及Debug逐层分配拒绝
// @return 是否全部通过
// */
// static bool LibuvAcceptAudit()
// {
//     if (!LibuvAcceptRound(0))
//     {
//         return false;
//     }
// #if defined(_WIN32) && defined(_DEBUG) && !defined(LIBUV_ACCEPT_NO_HEAP)
//     for (uint32_t mode = 1; mode <= 2; ++mode)
//     {
//         _CrtMemState before, after;
//         _CrtMemCheckpoint(&before);
//         if (!LibuvAcceptRound(mode))
//         {
//             return false;
//         }
//         _CrtMemCheckpoint(&after);
//         if (before.lCounts[_NORMAL_BLOCK] != after.lCounts[_NORMAL_BLOCK] || before.lSizes[_NORMAL_BLOCK] != after.lSizes[_NORMAL_BLOCK])
//         {
//             std::puts("Libuv accept heap delta FAIL");
//             return false;
//         }
//     }
// #endif
//     std::puts("Libuv accept recovery PASS");
//     return true;
// }
//
// #ifndef LIBUV_ACCEPT_NO_MAIN
// /** 独立测试入口，正式库中整段保持注释
// @return 零表示通过
// */
// int main()
// {
//     std::setvbuf(stdout, nullptr, _IONBF, 0);
//     return LibuvAcceptAudit() ? 0 : 1;
// }
// #endif
// LIBUV_ACCEPT_UNIT_END

// LIBUV_LIFECYCLE_UNIT_BEGIN
// #include "LibuvTcp.h"
// #include "LibuvChannel.h"
// #include <memory>
// #include <cstdio>
// #if defined(_WIN32) && defined(_DEBUG) && !defined(LIBUV_LIFECYCLE_NO_HEAP)
// #include <crtdbg.h>
// // 独立main唯一调用线程使用，不与其他CRT故障注入并行。
// static uint32_t s_lifecycleAllocations = 0;
// /** 拒绝全部普通堆分配，验证清理操作不依赖新的内存
// @param [in] operation CRT操作，其他参数沿用CRT原型
// @return 是否允许当前操作
// */
// static int __cdecl LifecycleMemoryHook(int operation, void*, size_t, int block, long, const unsigned char*, int)
// {
//     if ((operation == _HOOK_ALLOC || operation == _HOOK_REALLOC) && block == _NORMAL_BLOCK)
//     {
//         ++s_lifecycleAllocations;
//         return 0;
//     }
//     return 1;
// }
// #endif
//
// /** 验证未启动和已监听实例在内存耗尽时仍能停止、收敛和析构
// @return 全部通过为true
// */
// static bool LibuvLifecycleAudit()
// {
//     uint32_t checks = 0;
// #define LIFECYCLE_CHECK(value) { ++checks; if (!(value)) { std::printf("Libuv lifecycle line=%d FAIL\n", __LINE__); return false; } }
//     for (uint32_t mode = 0; mode < 3; ++mode)
//     {
//         std::unique_ptr<LibuvTcp> owner(new LibuvTcp());
//         if (mode != 0)
//         {
//             LIFECYCLE_CHECK(owner->initServer(0));
//         }
//         bool escaped = false;
//         bool released = false;
// #if defined(_WIN32) && defined(_DEBUG) && !defined(LIBUV_LIFECYCLE_NO_HEAP)
//         s_lifecycleAllocations = 0;
//         const _CRT_ALLOC_HOOK old = _CrtSetAllocHook(LifecycleMemoryHook);
// #endif
//         try
//         {
//             if (mode != 2)
//             {
//                 owner->stop();
//                 owner->stop();
//                 owner->send(nullptr);
//                 owner->close(nullptr);
//                 owner->loop();
//                 released = owner->loopPtr() == nullptr && !owner->isClient() && owner->m_libuv == nullptr;
//             }
//             else
//             {
//                 // 未调用loop，析构仍负责关闭初始化阶段已经取得的TCP和唤醒句柄。
//                 released = true;
//             }
//             owner.reset();
//         }
//         catch (...)
//         {
//             escaped = true;
//         }
// #if defined(_WIN32) && defined(_DEBUG) && !defined(LIBUV_LIFECYCLE_NO_HEAP)
//         _CrtSetAllocHook(old);
//         LIFECYCLE_CHECK(s_lifecycleAllocations == 0);
// #endif
//         LIFECYCLE_CHECK(!escaped && released && !owner);
//     }
//     std::printf("Libuv lifecycle cold/initialized/destructor checks=%u PASS\n", checks);
//     return true;
// #undef LIFECYCLE_CHECK
// }
//
// #ifndef LIBUV_LIFECYCLE_NO_MAIN
// /** 独立测试入口；正式库保留整段注释
// @return 零表示成功
// */
// int main()
// {
//     std::setvbuf(stdout, nullptr, _IONBF, 0);
//     return LibuvLifecycleAudit() ? 0 : 1;
// }
// #endif
// LIBUV_LIFECYCLE_UNIT_END

// LIBUV_ACCEPT_BOUNDARY_UNIT_BEGIN
// #include "LibuvChannelState.h"
// /** 记录接纳失败和恢复的配对事件；仅注释测试main实例化，不拥有运输对象
// */
// class LibuvAcceptBoundaryObserver : public LibuvChannelListener
// {
// public:
//     /** 初始化无连接、无数据的观察状态
//     */
//     LibuvAcceptBoundaryObserver();
//
//     /** 记录已经发布的非零连接号
//     @param [in] peer 本实例单调递增且不复用的连接号
//     */
//     virtual void onConnected(uint64_t peer);
//
//     /** 累计收到的二进制数据，不保留借用指针
//     @param [in] peer 已发布连接号
//     @param [in] data 借用到回调返回的非空数据
//     @param [in] size 本次字节数
//     */
//     virtual void onData(uint64_t peer, const uint8_t* data, uint32_t size);
//
//     /** 写完成无需驱动额外发送
//     @param [in] peer 已发布连接号
//     */
//     virtual void onWritable(uint64_t peer);
//
//     /** 核对终态只能属于已经发布的连接
//     @param [in] peer 关闭的连接号
//     @param [in] reason libuv错误码或主动关闭原因
//     */
//     virtual void onClosed(uint64_t peer, int32_t reason);
//
// public:
//     // 已发布的最近连接号，0表示尚无连接
//     uint64_t m_peer;
//     // 已发布的连接事件数
//     uint32_t m_connected;
//     // 已发布的关闭事件数
//     uint32_t m_closed;
//     // 按字节递增模式验证后的累计接收字节数
//     uint32_t m_received;
//     // 事件顺序、连接号及数据是否始终符合预期
//     bool m_valid;
// };
// #include <chrono>
// #include <cstdio>
//
// LibuvAcceptBoundaryObserver::LibuvAcceptBoundaryObserver() :
// m_peer(0),
// m_connected(0),
// m_closed(0),
// m_received(0),
// m_valid(true)
// {
//
// }
//
// void LibuvAcceptBoundaryObserver::onConnected(uint64_t peer)
// {
//     m_valid = m_valid && peer != 0 && peer > m_peer;
//     m_peer = peer;
//     ++m_connected;
// }
//
// void LibuvAcceptBoundaryObserver::onData(uint64_t peer, const uint8_t* data, uint32_t size)
// {
//     m_valid = m_valid && peer == m_peer && data != nullptr && size != 0;
//     for (uint32_t index = 0; index < size; ++index)
//     {
//         m_valid = m_valid && data[index] == static_cast<uint8_t>((m_received + index) % 251);
//     }
//     m_received += size;
// }
//
// void LibuvAcceptBoundaryObserver::onWritable(uint64_t peer)
// {
//     m_valid = m_valid && peer == m_peer;
// }
//
// void LibuvAcceptBoundaryObserver::onClosed(uint64_t peer, int32_t reason)
// {
//     (void)reason;
//     m_valid = m_valid && peer == m_peer && m_closed < m_connected;
//     ++m_closed;
// }
//
// /** 失败接纳不产生未发布连接的终态，随后正常连接仍能收发和收敛
// @return 零表示全部通过，非零定位失败检查点
// */
// int main()
// {
//     LibuvAcceptBoundaryObserver incoming;
//     LibuvAcceptBoundaryObserver outgoing;
//     LibuvChannelState server;
//     LibuvChannel client;
//     if (server.m_loopStatus != 0 || uv_tcp_init(&server.m_loop, &server.m_listener) != 0)
//     {
//         return 1;
//     }
//     server.m_listenerInitialized = true;
//     server.m_callback = &incoming;
//     server.m_listener.data = &server;
//     sockaddr_storage address;
//     if (server.parseAddress("127.0.0.1", 0, address) != 0 ||
//         uv_tcp_bind(&server.m_listener, reinterpret_cast<sockaddr*>(&address), 0) != 0 ||
//         uv_listen(reinterpret_cast<uv_stream_t*>(&server.m_listener), 16, &LibuvChannelState::acceptPeer) != 0)
//     {
//         return 2;
//     }
//     LibuvChannelState::acceptPeer(reinterpret_cast<uv_stream_t*>(&server.m_listener), UV_ECANCELED);
//     if (!server.m_peers.empty() || server.m_nextPeer != 1)
//     {
//         return 3;
//     }
//     for (uint32_t index = 0; index < 128; ++index)
//     {
//         // 无待接连接时uv_accept真实返回UV_EAGAIN，不替换libuv接口。
//         LibuvChannelState::acceptPeer(reinterpret_cast<uv_stream_t*>(&server.m_listener), 0);
//         uv_run(&server.m_loop, UV_RUN_NOWAIT);
//         if (!server.m_peers.empty() || incoming.m_connected != 0 || incoming.m_closed != 0 || !incoming.m_valid)
//         {
//             std::printf("unpublished accept closed=%u connected=%u round=%u FAIL\n",
//                 incoming.m_closed, incoming.m_connected, index);
//             return 4;
//         }
//     }
//     int length = sizeof(address);
//     if (uv_tcp_getsockname(&server.m_listener, reinterpret_cast<sockaddr*>(&address), &length) != 0)
//     {
//         return 5;
//     }
//     uint64_t peer = 0;
//     const uint32_t port = ntohs(reinterpret_cast<sockaddr_in*>(&address)->sin_port);
//     if (client.connect("127.0.0.1", port, &outgoing, peer) != 0)
//     {
//         return 6;
//     }
//     auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
//     while ((incoming.m_connected != 1 || outgoing.m_connected != 1) && std::chrono::steady_clock::now() < deadline)
//     {
//         uv_run(&server.m_loop, UV_RUN_NOWAIT);
//         client.poll();
//         uv_sleep(1);
//     }
//     if (incoming.m_connected != 1 || outgoing.m_connected != 1)
//     {
//         return 7;
//     }
//     const uint8_t data[] = { 0, 1, 2, 3 };
//     if (client.write(peer, data, sizeof(data)) != 0)
//     {
//         return 8;
//     }
//     deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
//     while (incoming.m_received != sizeof(data) && std::chrono::steady_clock::now() < deadline)
//     {
//         uv_run(&server.m_loop, UV_RUN_NOWAIT);
//         client.poll();
//         uv_sleep(1);
//     }
//     server.shutdown();
//     client.shutdown();
//     deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
//     while ((!server.m_peers.empty() || client.peerCount() != 0) && std::chrono::steady_clock::now() < deadline)
//     {
//         uv_run(&server.m_loop, UV_RUN_NOWAIT);
//         client.poll();
//         uv_sleep(1);
//     }
//     if (!server.m_peers.empty() || client.peerCount() != 0 || !incoming.m_valid || !outgoing.m_valid ||
//         incoming.m_closed != 1 || outgoing.m_closed != 1 || incoming.m_received != sizeof(data))
//     {
//         return 9;
//     }
//     std::puts("Libuv accept unpublished=128 recovered=1 bytes=4 closed=1 PASS");
//     return 0;
// }
// LIBUV_ACCEPT_BOUNDARY_UNIT_END

int32_t LibuvTcp::hostCreate(void* providerContext, void* observerContext, LibuvHostConnected connected,
    LibuvHostData data, LibuvHostWritable writable, LibuvHostClosed closed, void** instance)
{
    (void)providerContext;
    if (instance == nullptr)
    {
        return -1;
    }
    *instance = nullptr;
    if (connected == nullptr || data == nullptr || writable == nullptr || closed == nullptr)
    {
        return -1;
    }
    try
    {
        *instance = new LibuvHostState(observerContext, connected, data, writable, closed);
        return 0;
    }
    catch (...)
    {
        return -1;
    }
}

int32_t LibuvTcp::hostListen(void* instance, const char* address, uint32_t port, uint32_t maxPeers)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr || state->m_stopping || state->m_depth != 0 || state->m_polling)
    {
        return -1;
    }
    try
    {
        return state->listen(address, port, maxPeers);
    }
    catch (...)
    {
        return -1;
    }
}

int32_t LibuvTcp::hostConnect(void* instance, const char* address, uint32_t port, uint64_t* peer)
{
    if (peer == nullptr)
    {
        return -1;
    }
    *peer = 0;
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr || state->m_stopping || state->m_depth != 0 || state->m_polling)
    {
        return -1;
    }
    try
    {
        return state->connect(address, port, *peer);
    }
    catch (...)
    {
        *peer = 0;
        return -1;
    }
}

int32_t LibuvTcp::hostPoll(void* instance)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr || state->m_depth != 0 || state->m_polling)
    {
        return -1;
    }
    state->m_polling = true;
    int32_t result = -1;
    try
    {
        const bool active = state->m_channel.poll();
        // uv_run在全部读取暂停时可返回0，但暂停的TCP资源仍由本实例拥有
        result = active || state->m_channel.peerCount() != 0 ? 1 : 0;
    }
    catch (...)
    {
        state->m_stopping = true;
        // 保留实例，由调用者在外层收敛，不在可能的回调栈上释放
    }
    state->m_polling = false;
    return result;
}

int32_t LibuvTcp::hostWrite(void* instance, uint64_t peer, const uint8_t* data, uint32_t size)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr || state->m_stopping)
    {
        return -1;
    }
    try
    {
        const int32_t result = state->m_channel.write(peer, data, size);
        return LibuvChannel::isBackpressure(result) ? 1 : result;
    }
    catch (...)
    {
        return -1;
    }
}

int32_t LibuvTcp::hostPauseRead(void* instance, uint64_t peer, int32_t paused)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr || state->m_stopping || (paused != 0 && paused != 1))
    {
        return -1;
    }
    try
    {
        return state->m_channel.pauseRead(peer, paused != 0);
    }
    catch (...)
    {
        return -1;
    }
}

int32_t LibuvTcp::hostClose(void* instance, uint64_t peer, int32_t reason)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr)
    {
        return -1;
    }
    try
    {
        state->m_channel.close(peer, reason);
        return 0;
    }
    catch (...)
    {
        return -1;
    }
}

int32_t LibuvTcp::hostShutdown(void* instance)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr)
    {
        return -1;
    }
    state->m_stopping = true;
    try
    {
        state->m_channel.shutdown();
        return 0;
    }
    catch (...)
    {
        return -1;
    }
}

int32_t LibuvTcp::hostDestroy(void* instance)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr)
    {
        return 0;
    }
    if (state->m_depth != 0 || state->m_polling)
    {
        return -1;
    }
    // shutdown期间的终态也不得重入销毁；只有完全收敛后才释放监听者
    state->m_polling = true;
    const int32_t result = LibuvTcp::hostShutdown(instance);
    state->m_polling = false;
    if (result != 0)
    {
        return result;
    }
    delete state;
    return 0;
}

uint32_t LibuvTcp::hostLocalPort(void* instance)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    return state == nullptr ? 0 : state->m_channel.localPort();
}

uint32_t LibuvTcp::hostPeerCount(void* instance)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    return state == nullptr ? 0 : state->m_channel.peerCount();
}

// LIBUV_HOST_UNIT_TEST_BEGIN
// #include <vector>
// #include <chrono>
// #include <cstdio>
// #include <cstring>
//
// // 仅注释main放开时使用，测试观测值不参与正式运行
// struct LibuvHostTestObserver
// {
// public:
//     // 被测运输，回调中用于验证非法重入被拒绝
//     void* m_instance;
//     // 首次建立的连接号
//     uint64_t m_peer;
//     // 接纳连接事件总数
//     uint32_t m_connected;
//     // 唯一终态总数
//     uint32_t m_closed;
//     // 本地写完成次数
//     uint32_t m_writable;
//     // 成功拒绝回调内destroy/poll次数
//     uint32_t m_reentryRejected;
//     // 每次输入长度/peer是否合法
//     bool m_valid;
//     // 是否在连接事件请求停止
//     bool m_stopOnConnect;
//     // 是否模拟上层抛出异常
//     bool m_throwOnConnect;
//     // 接收到的真实字节副本
//     std::vector<uint8_t> m_bytes;
//
// public:
//     /** 初始化全部观测值，不创建运输
//     */
//     LibuvHostTestObserver();
// };
//
// LibuvHostTestObserver::LibuvHostTestObserver() :
// m_instance(nullptr),
// m_peer(0),
// m_connected(0),
// m_closed(0),
// m_writable(0),
// m_reentryRejected(0),
// m_valid(true),
// m_stopOnConnect(false),
// m_throwOnConnect(false)
// {
//
// }
//
// /** 只供测试的单调毫秒截止时间
// @return 当前单调毫秒
// */
// static uint64_t HostTestNow()
// {
//     return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
//         std::chrono::steady_clock::now().time_since_epoch()).count());
// }
//
// /** 记录连接并尝试非法重入，验证实例仍保持有效
// @param [in] context 测试观察者
// @param [in] peer 新代次
// */
// static void HostTestConnected(void* context, uint64_t peer)
// {
//     LibuvHostTestObserver* observer = static_cast<LibuvHostTestObserver*>(context);
//     observer->m_peer = peer;
//     ++observer->m_connected;
//     observer->m_valid = observer->m_valid && peer != 0;
//     if (LibuvTcp::hostDestroy(observer->m_instance) < 0 && LibuvTcp::hostPoll(observer->m_instance) < 0)
//     {
//         ++observer->m_reentryRejected;
//     }
//     if (observer->m_stopOnConnect)
//     {
//         LibuvTcp::hostShutdown(observer->m_instance);
//     }
//     if (observer->m_throwOnConnect)
//     {
//         throw 1;
//     }
// }
//
// /** 验证数据范围并复制输入
// @param [in] context 测试观察者
// @param [in] peer 输入代次
// @param [in] data 借用输入
// @param [in] size 借用长度
// */
// static void HostTestData(void* context, uint64_t peer, const uint8_t* data, uint32_t size)
// {
//     LibuvHostTestObserver* observer = static_cast<LibuvHostTestObserver*>(context);
//     observer->m_valid = observer->m_valid && peer == observer->m_peer && data != nullptr && size != 0 && size <= 16384;
//     observer->m_bytes.insert(observer->m_bytes.end(), data, data + size);
// }
//
// /** 统计真实本地写完成
// @param [in] context 测试观察者
// @param [in] peer 写代次
// */
// static void HostTestWritable(void* context, uint64_t peer)
// {
//     LibuvHostTestObserver* observer = static_cast<LibuvHostTestObserver*>(context);
//     observer->m_valid = observer->m_valid && peer == observer->m_peer;
//     ++observer->m_writable;
// }
//
// /** 记录唯一终态并验证回调期间仍不能销毁
// @param [in] context 测试观察者
// @param [in] peer 终结代次
// @param [in] reason 终态原因
// */
// static void HostTestClosed(void* context, uint64_t peer, int32_t reason)
// {
//     (void)reason;
//     LibuvHostTestObserver* observer = static_cast<LibuvHostTestObserver*>(context);
//     observer->m_valid = observer->m_valid && peer == observer->m_peer;
//     ++observer->m_closed;
//     if (LibuvTcp::hostDestroy(observer->m_instance) < 0 && LibuvTcp::hostPoll(observer->m_instance) < 0)
//     {
//         ++observer->m_reentryRejected;
//     }
// }
//
// /** 独立宿主测试入口，覆盖回调重入、背压、暂停、异常及资源收敛
// @return 零表示全部通过，非零表示断言失败
// */
// int main()
// {
//     uint32_t assertions = 0;
// #define HOST_CHECK(condition) { ++assertions; if (!(condition)) { std::printf("FAIL line %u: %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
//     HOST_CHECK(LibuvTcp::hostDestroy(nullptr) == 0);
//     HOST_CHECK(LibuvTcp::hostPoll(nullptr) < 0);
//     HOST_CHECK(LibuvTcp::hostWrite(nullptr, 1, nullptr, 0) < 0);
//     HOST_CHECK(LibuvTcp::hostPauseRead(nullptr, 1, 0) < 0);
//     HOST_CHECK(LibuvTcp::hostShutdown(nullptr) < 0);
//     HOST_CHECK(LibuvTcp::hostClose(nullptr, 1, 0) < 0);
//     HOST_CHECK(LibuvTcp::hostLocalPort(nullptr) == 0 && LibuvTcp::hostPeerCount(nullptr) == 0);
//     void* invalid = reinterpret_cast<void*>(1);
//     HOST_CHECK(LibuvTcp::hostCreate(nullptr, nullptr, nullptr, HostTestData, HostTestWritable, HostTestClosed, &invalid) < 0);
//     HOST_CHECK(invalid == nullptr);
//     HOST_CHECK(LibuvTcp::hostCreate(nullptr, nullptr, HostTestConnected, HostTestData, HostTestWritable, HostTestClosed, nullptr) < 0);
//     for (uint32_t mode = 0; mode < 3; ++mode)
//     {
//         LibuvHostTestObserver server;
//         LibuvHostTestObserver client;
//         client.m_stopOnConnect = mode == 1;
//         client.m_throwOnConnect = mode == 2;
//         HOST_CHECK(LibuvTcp::hostCreate(nullptr, &server, HostTestConnected, HostTestData, HostTestWritable, HostTestClosed, &server.m_instance) == 0);
//         HOST_CHECK(LibuvTcp::hostCreate(nullptr, &client, HostTestConnected, HostTestData, HostTestWritable, HostTestClosed, &client.m_instance) == 0);
//         HOST_CHECK(LibuvTcp::hostListen(server.m_instance, "127.0.0.1", 0, 2) == 0);
//         HOST_CHECK(LibuvTcp::hostLocalPort(server.m_instance) != 0);
//         uint64_t peer = 999;
//         HOST_CHECK(LibuvTcp::hostConnect(client.m_instance, "bad-address", 9, &peer) < 0 && peer == 0);
//         HOST_CHECK(LibuvTcp::hostConnect(client.m_instance, "127.0.0.1", LibuvTcp::hostLocalPort(server.m_instance), &peer) == 0 && peer != 0);
//         HOST_CHECK(client.m_connected == 0 && server.m_connected == 0);
//         const uint64_t connectedDeadline = HostTestNow() + 5000;
//         while ((client.m_connected == 0 || server.m_connected == 0) && HostTestNow() < connectedDeadline)
//         {
//             HOST_CHECK(LibuvTcp::hostPoll(client.m_instance) >= 0);
//             HOST_CHECK(LibuvTcp::hostPoll(server.m_instance) >= 0);
//         }
//         HOST_CHECK(client.m_connected == 1 && server.m_connected == 1);
//         HOST_CHECK(client.m_reentryRejected >= 1 && server.m_reentryRejected >= 1);
//         if (mode == 0)
//         {
//             HOST_CHECK(LibuvTcp::hostPauseRead(server.m_instance, server.m_peer, 2) < 0);
//             HOST_CHECK(LibuvTcp::hostPauseRead(server.m_instance, server.m_peer, 1) == 0);
//             HOST_CHECK(LibuvTcp::hostPauseRead(client.m_instance, peer, 1) == 0);
//             HOST_CHECK(LibuvTcp::hostPoll(client.m_instance) == 1);
//             HOST_CHECK(LibuvTcp::hostPoll(server.m_instance) == 1);
//             std::vector<uint8_t> first(32768, 0x35);
//             std::vector<uint8_t> second(32768, 0xc9);
//             HOST_CHECK(LibuvTcp::hostWrite(client.m_instance, peer, nullptr, 1) < 0);
//             HOST_CHECK(LibuvTcp::hostWrite(client.m_instance, peer, first.data(), 32769) < 0);
//             HOST_CHECK(LibuvTcp::hostWrite(client.m_instance, peer, first.data(), 32768) == 0);
//             HOST_CHECK(LibuvTcp::hostWrite(client.m_instance, peer, second.data(), 32768) == 0);
//             HOST_CHECK(LibuvTcp::hostWrite(client.m_instance, peer, first.data(), 1) == 1);
//             first.assign(32768, 0);
//             second.assign(32768, 0);
//             for (uint32_t index = 0; index < 16; ++index)
//             {
//                 HOST_CHECK(LibuvTcp::hostPoll(client.m_instance) >= 0);
//                 HOST_CHECK(LibuvTcp::hostPoll(server.m_instance) >= 0);
//             }
//             HOST_CHECK(server.m_bytes.empty());
//             HOST_CHECK(LibuvTcp::hostPauseRead(server.m_instance, server.m_peer, 0) == 0);
//             const uint64_t receivedDeadline = HostTestNow() + 5000;
//             while (server.m_bytes.size() < 65536 && HostTestNow() < receivedDeadline)
//             {
//                 HOST_CHECK(LibuvTcp::hostPoll(client.m_instance) >= 0);
//                 HOST_CHECK(LibuvTcp::hostPoll(server.m_instance) >= 0);
//             }
//             HOST_CHECK(server.m_bytes.size() == 65536 && client.m_writable == 2);
//             for (size_t index = 0; index < server.m_bytes.size(); ++index)
//             {
//                 HOST_CHECK(server.m_bytes[index] == (index < 32768 ? 0x35 : 0xc9));
//             }
//             HOST_CHECK(LibuvTcp::hostClose(client.m_instance, peer, 19) == 0);
//             HOST_CHECK(LibuvTcp::hostClose(client.m_instance, peer, 20) == 0);
//         }
//         const uint64_t closedDeadline = HostTestNow() + 5000;
//         while ((client.m_closed == 0 || server.m_closed == 0) && HostTestNow() < closedDeadline)
//         {
//             HOST_CHECK(LibuvTcp::hostPoll(client.m_instance) >= 0);
//             HOST_CHECK(LibuvTcp::hostPoll(server.m_instance) >= 0);
//         }
//         HOST_CHECK(client.m_closed == 1 && server.m_closed == 1);
//         HOST_CHECK(client.m_valid && server.m_valid);
//         HOST_CHECK(LibuvTcp::hostPeerCount(client.m_instance) == 0 && LibuvTcp::hostPeerCount(server.m_instance) == 0);
//         HOST_CHECK(LibuvTcp::hostShutdown(client.m_instance) == 0);
//         HOST_CHECK(LibuvTcp::hostWrite(client.m_instance, peer, nullptr, 0) < 0);
//         HOST_CHECK(LibuvTcp::hostDestroy(client.m_instance) == 0);
//         HOST_CHECK(LibuvTcp::hostDestroy(server.m_instance) == 0);
//         HOST_CHECK(client.m_closed == 1 && server.m_closed == 1);
//     }
//     std::printf("LibuvHost PASS %u assertions\n", assertions);
//     return 0;
// #undef HOST_CHECK
// }
// LIBUV_HOST_UNIT_TEST_END