#include "LibuvTcpTest.h"
#include "LibuvTcpTestModels.h"
#include "LibuvTcpBenchmark.h"
#include "LibuvTcpPerformance.h"
#include "LibuvTcpPublicTest.h"
// 白盒布局只由测试包含，正式 API 不需要暴露这些文件。
#include "LibuvTcp/LibuvBase.h"
#include "LibuvTcp/LibuvChannelModels.h"
#include "LibuvTcp/LibuvChannelState.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <cstring>
#include <cstdio>
#include <new>
#include <cstdlib>
#include <cerrno>
#include <map>
#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

std::atomic<void*> LibuvTcpTest::m_watchedArray(nullptr);
std::atomic<size_t> LibuvTcpTest::m_arrayDeletes(0);
std::atomic<size_t> LibuvTcpTest::m_failAllocationSize(0);
std::atomic<uint32_t> LibuvTcpTest::m_failAllocationCount(0);

#ifndef LIBUV_TEST_BENCHMARK
/** 测试进程的定点分配故障，只在显式选定大小和次数时生效
@param [in] size 申请字节数
@return 分配存储，指定故障或资源不足时抛出 bad_alloc
*/
void* operator new(size_t size)
{
    uint32_t remaining = LibuvTcpTest::m_failAllocationCount.load();
    if (size == LibuvTcpTest::m_failAllocationSize.load())
    {
        while (remaining != 0)
        {
            if (LibuvTcpTest::m_failAllocationCount.compare_exchange_weak(remaining, remaining - 1))
            {
                throw std::bad_alloc();
            }
        }
    }
    void* memory = std::malloc(size == 0 ? 1 : size);
    if (memory == nullptr)
    {
        throw std::bad_alloc();
    }
    return memory;
}

/** 与测试进程标量分配配对，不影响正式库源文件
@param [in] address 分配存储或空
*/
void operator delete(void* address) throw()
{
    std::free(address);
}

/** 数组分配仍委托标准分配器，供静态测试程序观察配对释放
@param [in] size 申请字节数
@return 独占存储；分配失败沿用标准 bad_alloc
*/
void* operator new[](size_t size)
{
    return ::operator new(size);
}

/** 记录被观察数组并交还标准分配器
@param [in] address 数组存储或空
*/
void operator delete[](void* address) throw()
{
    LibuvTcpTest::observeDelete(address);
    ::operator delete(address);
}
#endif

void LibuvTcpTest::observeDelete(void* address)
{
    if (address != nullptr && m_watchedArray.load() == address)
    {
        m_arrayDeletes.fetch_add(1);
    }
}

/** 测试消费者从实际发送通道取走一项，不假定全部数据位于并发队列
@param [in,out] state 独占消费职责的状态
@param [out] pending 成功后由测试负责数组与预算
@return 是否取得一项
*/
static bool PopSubmitted(LibuvBase* state, LibuvPendingSend* pending)
{
    size_t taken = 0;
    return state->popSendPrefix(pending, 1, taken) && taken == 1;
}

/** 在尚未驱动的连接中构造真实队列饱和，不替换 push 实现
@param [in,out] tcp 当前独占测试实例
@param [in] closes 是否填充取消队列，默认填充数据队列
@return 成功登记且实际遇到容量拒绝
*/
static bool FillQueue(LibuvTcp& tcp, bool closes = false)
{
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    LibuvPendingSend pending;
    if (!closes)
    {
        // 先填满单写通道但不领取拥有者，后续首个发送线程也必须遇到实际背压。
        for (uint32_t index = 0; index < 1024; ++index)
        {
            if (!tcp.m_libuv->m_sendLane.push(pending))
            {
                return false;
            }
        }
        // 附加通道也必须实际填满，否则后来的生产者尚有可用容量。
        for (uint32_t slot = 0; slot < tcp.m_libuv->m_settings.m_send.m_producerCacheCapacity; ++slot)
        {
            LibuvSendLane& lane = tcp.m_libuv->m_sendProducers[slot].m_lane;
            for (uint32_t index = 0; index < lane.m_capacity; ++index)
            {
                if (!lane.push(pending))
                {
                    return false;
                }
            }
        }
    }
    for (size_t index = 0; index < 16384; ++index)
    {
#ifndef LIBUV_TEST_BASELINE
        const LockFreeQueueResult result = (closes ? tcp.m_libuv->m_closeQueue : tcp.m_libuv->m_queue)->tryPush(pending, true, false);
#else
        (void)closes;
        const LockFreeQueueResult result = tcp.m_libuv->m_queue->tryPush(pending, true, false);
#endif
        if (result != LockFreeQueueSuccess)
        {
            return result == LockFreeQueueCapacityLimit || result == LockFreeQueueResourceUnavailable;
        }
    }
    return false;
}

/** 满队列且没有循环执行者时拒绝非法等待，普通 send 仍恰好释放数组
@return 是否满足完整失败后置条件
*/
static bool TestRejectedSend()
{
    LibuvTcp tcp;
    if (!FillQueue(tcp))
    {
        return false;
    }
    const int32_t length = 1;
    char* buffer = tcp.memoryForSend(static_cast<int32_t>(sizeof(uv_tcp_t*) + sizeof(length) + length));
    uv_tcp_t* target = tcp.m_libuv->mainTcp();
    std::memcpy(buffer, &target, sizeof(target));
    std::memcpy(buffer + sizeof(target), &length, sizeof(length));
    buffer[sizeof(target) + sizeof(length)] = 'x';
    LibuvTcpTest::m_arrayDeletes.store(0);
    LibuvTcpTest::m_watchedArray.store(buffer);
    tcp.send(buffer);
    const size_t deleted = LibuvTcpTest::m_arrayDeletes.load();
    const bool passed = deleted == 1 && !tcp.m_libuv->m_stopping.load();
    LibuvTcpTest::m_watchedArray.store(nullptr);
    // 旧基准未接纳也未释放；测试自行回收，不能把清理当作被测库通过。
    if (deleted == 0)
    {
        delete[] buffer;
    }
    return passed;
}

/** 验证关闭不能因队列满被静默丢弃；无法排队时请求全局收敛
@return 是否产生可观察停止请求
*/
static bool TestRejectedClose()
{
    LibuvTcp tcp;
    if (!FillQueue(tcp, true))
    {
        return false;
    }
    tcp.close(tcp.m_libuv->mainTcp());
    return tcp.m_libuv->m_stopping.load();
}

/** 同时挂起多个读缓冲，回调归还前不能复用；空读也必须归还
@return 每次借用是否独立且长度上限正确
*/
static bool TestReadBuffers()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1))
    {
        return false;
    }
    uv_buf_t buffers[12] = {};
    uv_tcp_t* target = tcp.m_libuv->mainTcp();
    bool passed = true;
    for (size_t index = 0; index < 12; ++index)
    {
        LibuvBase::AllocateRead(reinterpret_cast<uv_handle_t*>(target), 131072, &buffers[index]);
        passed = buffers[index].base != nullptr && buffers[index].len == 65536 && passed;
        if (buffers[index].base != nullptr)
        {
            std::memset(buffers[index].base, static_cast<int>(index), buffers[index].len);
        }
        for (size_t prior = 0; prior < index; ++prior)
        {
            passed = buffers[prior].base != buffers[index].base && passed;
        }
    }
    for (size_t index = 0; index < 12; ++index)
    {
        if (buffers[index].base != nullptr)
        {
            for (size_t offset = 0; offset < buffers[index].len; ++offset)
            {
                passed = static_cast<unsigned char>(buffers[index].base[offset]) == index && passed;
            }
        }
        LibuvBase::Read(reinterpret_cast<uv_stream_t*>(target), 0, &buffers[index]);
    }
    for (size_t repeat = 0; repeat < 1000; ++repeat)
    {
        uv_buf_t buffer = {};
        LibuvBase::AllocateRead(reinterpret_cast<uv_handle_t*>(target), 64, &buffer);
        passed = buffer.base != nullptr && buffer.len == 64 && passed;
        LibuvBase::Read(reinterpret_cast<uv_stream_t*>(target), 0, &buffer);
    }
    uv_buf_t empty = {};
    LibuvBase::AllocateRead(reinterpret_cast<uv_handle_t*>(target), 0, &empty);
    passed = empty.base == nullptr && empty.len == 0 && passed;
    LibuvBase::Read(reinterpret_cast<uv_stream_t*>(target), 0, &empty);
    return passed;
}

#ifndef LIBUV_TEST_BASELINE
/** 验证发送总预算不会越界，拒绝后无隐含预留
@return 边界、回滚和余额是否正确
*/
static bool TestSendBudget()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1))
    {
        return false;
    }
    LibuvBase* state = tcp.m_libuv;
    const uint32_t limit = 64U * 1024U * 1024U;
    bool passed = state->tryReserveSend(0, true) == UV_EINVAL &&
        state->tryReserveSend(0xffffffffU, true) == UV_EINVAL &&
        state->tryReserveSend(limit, true) == 0 && state->tryReserveSend(1, true) == UV_ENOBUFS;
    passed = state->m_pendingBytes.load() == limit && passed;
    state->releaseSend(limit);
    passed = state->m_pendingBytes.load() == 0 && passed;
    return passed;
}

/** 同一调用方顺序生成有效提交；容量和目标由测试实际初始化
@param [in,out] tcp 本次独占实例
*/
static void SubmitByte(LibuvTcp& tcp)
{
    const int32_t length = 1;
    uv_tcp_t* target = tcp.m_libuv->mainTcp();
    char* buffer = tcp.memoryForSend(static_cast<int32_t>(sizeof(target) + 4 + length));
    std::memcpy(buffer, &target, sizeof(target));
    std::memcpy(buffer + sizeof(target), &length, 4);
    buffer[sizeof(target) + 4] = 'x';
    tcp.send(buffer);
}

/** 批量消费受到每轮1024项限制，停止后仍完整回收已经取出的前缀
@return 预算是否随实际消费归还且最终归零
*/
static bool TestStoppedPrefix()
{
    LibuvTcp tcp;
    // 关闭附加通道，明确验证主通道1024项加共享队列176项的停止回收边界
    LibuvSendPoolConfig send;
    send.m_producerCacheCapacity = 0;
    if (tcp.configureSendPool(send) != 0)
    {
        return false;
    }
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    for (size_t index = 0; index < 1024; ++index)
    {
        SubmitByte(tcp);
    }
    LibuvTestThread sharedProducer;
    if (!sharedProducer.start([&]()
    {
        for (size_t index = 0; index < 176; ++index)
        {
            SubmitByte(tcp);
        }
    }, [&]() { tcp.stop(); }) || !sharedProducer.join())
    {
        return false;
    }
    LibuvBase* state = tcp.m_libuv;
    const uint32_t charge = static_cast<uint32_t>(sizeof(uv_tcp_t*) + 5 + sizeof(LibuvPendingWrite));
    bool passed = state->m_pendingBytes.load() == charge * 1200;
    tcp.stop();
    LibuvBase::SendPending(state->m_asyncHandle);
    passed = state->m_pendingBytes.load() == charge * 176 && passed;
    LibuvBase::SendPending(state->m_asyncHandle);
    return state->m_pendingBytes.load() == 0 && passed;
}

/** 同一地址登记为新代后，旧代排队数据只能回收，不能投给新连接
@return 旧代预算已归还且没有尝试写未连通的新代连接
*/
static bool TestStaleGeneration()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    SubmitByte(tcp);
    LibuvBase* state = tcp.m_libuv;
    if (!state->registerTcp(state->mainTcp()))
    {
        return false;
    }
    LibuvBase::SendPending(state->m_asyncHandle);
    return state->m_pendingBytes.load() == 0 && !state->m_stopping.load();
}
#endif

/** 在同一 owner 上驱动一对 host，等待实际条件或超时
@param [in,out] server 服务端观测器
@param [in,out] client 客户端观测器
@param [in] ready 必须满足的实际状态
@return 是否满足条件且期间无契约错误
*/
static bool PollHosts(LibuvTestHost& server, LibuvTestHost& client, const std::function<bool()>& ready)
{
    const uint64_t deadline = uv_hrtime() + 5000000000ULL;
    while (!ready() && !server.m_failed && !client.m_failed && uv_hrtime() < deadline)
    {
        if (LibuvTcp::hostPoll(server.m_instance) < 0 || LibuvTcp::hostPoll(client.m_instance) < 0)
        {
            return false;
        }
        uv_sleep(0);
    }
    return ready() && !server.m_failed && !client.m_failed;
}

/** 通过公开 host 入口建立回环连接
@param [in,out] server 空服务端观测器
@param [in,out] client 空客户端观测器
@return 是否连通
*/
static bool ConnectHosts(LibuvTestHost& server, LibuvTestHost& client)
{
    uint64_t peer = 0;
    return server.initialize() && client.initialize() &&
        LibuvTcp::hostListen(server.m_instance, "127.0.0.1", 0, 1) == 0 &&
        LibuvTcp::hostConnect(client.m_instance, "127.0.0.1", LibuvTcp::hostLocalPort(server.m_instance), &peer) == 0 &&
        peer != 0 && PollHosts(server, client, [&]() { return server.m_connected == 1 && client.m_connected == 1; });
}

/** 验证空参数、重入、关闭幂等和一次性销毁契约
@return 结果与实际资源状态是否一致
*/
static bool TestHostContracts()
{
    LibuvTestHost server;
    LibuvTestHost client;
    void* invalid = reinterpret_cast<void*>(1);
    if (LibuvTcp::hostCreate(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &invalid) >= 0 || invalid != nullptr ||
        LibuvTcp::hostDestroy(nullptr) != 0 || LibuvTcp::hostPoll(nullptr) >= 0 ||
        LibuvTcp::hostLocalPort(nullptr) != 0 || LibuvTcp::hostPeerCount(nullptr) != 0 ||
        !ConnectHosts(server, client))
    {
        return false;
    }
    bool passed = LibuvTcp::hostPauseRead(client.m_instance, client.m_peer, 2) < 0 &&
        LibuvTcp::hostWrite(client.m_instance, client.m_peer, nullptr, 1) < 0 &&
        LibuvTcp::hostWrite(client.m_instance, client.m_peer, nullptr, 0) == 0;
    passed = LibuvTcp::hostClose(client.m_instance, client.m_peer, 0) == 0 && passed;
    passed = LibuvTcp::hostClose(client.m_instance, client.m_peer, 0) == 0 && passed;
    passed = PollHosts(server, client, [&]() { return server.m_closed == 1 && client.m_closed == 1; }) && passed;
    passed = LibuvTcp::hostPeerCount(server.m_instance) == 0 && LibuvTcp::hostPeerCount(client.m_instance) == 0 && passed;
    passed = server.finish() && passed;
    passed = client.finish() && passed;
    return passed && !server.m_failed && !client.m_failed && server.m_closed == 1 && client.m_closed == 1;
}

/** 验证精确背压、恢复与暂停读取，不把写接纳当成对方接收
@return 未接纳消息是否无副作用且已接纳数据完整送达
*/
static bool TestHostBackpressure()
{
    LibuvTestHost server;
    LibuvTestHost client;
    if (!ConnectHosts(server, client))
    {
        return false;
    }
    const uint8_t byte = 0x5A;
    bool passed = true;
    for (uint32_t index = 0; index < 64; ++index)
    {
        passed = LibuvTcp::hostWrite(client.m_instance, client.m_peer, &byte, 1) == 0 && passed;
    }
    passed = LibuvTcp::hostWrite(client.m_instance, client.m_peer, &byte, 1) == 1 && passed;
    passed = PollHosts(server, client, [&]() { return server.m_received == 64 && client.m_written == 64; }) && passed;
    passed = LibuvTcp::hostPauseRead(server.m_instance, server.m_peer, 1) == 0 && passed;
    passed = LibuvTcp::hostWrite(client.m_instance, client.m_peer, &byte, 1) == 0 && passed;
    passed = PollHosts(server, client, [&]() { return client.m_written == 65; }) && passed;
    for (uint32_t tick = 0; tick < 100; ++tick)
    {
        LibuvTcp::hostPoll(server.m_instance);
        LibuvTcp::hostPoll(client.m_instance);
        passed = server.m_received == 64 && passed;
    }
    passed = LibuvTcp::hostPauseRead(server.m_instance, server.m_peer, 0) == 0 && passed;
    passed = PollHosts(server, client, [&]() { return server.m_received == 65; }) && passed;
    passed = client.finish() && passed;
    passed = server.finish() && passed;
    return passed && client.m_closed == 1 && server.m_closed == 1 && !server.m_failed && !client.m_failed;
}

/** 分别在四种业务回调注入异常，异常不得穿出 libuv 或破坏配对终态
@return 所有异常路径是否完整收敛
*/
static bool TestHostExceptions()
{
    for (uint32_t stage = 1; stage <= 4; ++stage)
    {
        LibuvTestHost server;
        LibuvTestHost client;
        client.m_throwOn = stage;
        if (!ConnectHosts(server, client))
        {
            return false;
        }
        const uint8_t byte = 0x5A;
        if (stage == 2 && LibuvTcp::hostWrite(server.m_instance, server.m_peer, &byte, 1) != 0)
        {
            return false;
        }
        if (stage == 3 && LibuvTcp::hostWrite(client.m_instance, client.m_peer, &byte, 1) != 0)
        {
            return false;
        }
        if (stage == 4)
        {
            LibuvTcp::hostClose(client.m_instance, client.m_peer, 0);
        }
        if (!PollHosts(server, client, [&]() { return server.m_closed == 1 && client.m_closed == 1; }) ||
            !client.finish() || !server.finish() || server.m_closed != 1 || client.m_closed != 1)
        {
            return false;
        }
    }
    return true;
}

/** 检查异步循环反复创建与提前停止后内核句柄不累积
@return 反复生命周期是否成功且句柄回到预热基线
*/
static bool TestRepeatedLifecycle()
{
    {
        LibuvTcp warmup;
        if (!warmup.initClient("127.0.0.1", 1))
        {
            return false;
        }
        warmup.stop();
        warmup.loop();
    }
#ifdef _WIN32
    DWORD before = 0;
    if (!GetProcessHandleCount(GetCurrentProcess(), &before))
    {
        return false;
    }
#endif
    for (uint32_t round = 0; round < 256; ++round)
    {
        LibuvTcp tcp;
        if (!tcp.initClient("127.0.0.1", 1) || tcp.initClient("127.0.0.1", 1))
        {
            return false;
        }
        tcp.stop();
        tcp.stop();
        if ((round & 1) == 0)
        {
            tcp.loop();
            tcp.loop();
            if (tcp.loopPtr() != nullptr || tcp.isClient())
            {
                return false;
            }
        }
    }
#ifdef _WIN32
    DWORD after = 0;
    const bool read = GetProcessHandleCount(GetCurrentProcess(), &after) != 0;
    std::printf("handles before=%lu after=%lu\n", before, after);
    return read && after <= before;
#else
    return true;
#endif
}

/** 固定交错验证封闭新借用后，回收等待旧借用退出且不允许再次进入
@return 关闭和归还顺序是否满足生命周期边界
*/
static bool TestBorrowClose()
{
    LibuvLifecycleGate gate;
    gate.open();
    if (!gate.acquire())
    {
        return false;
    }
    std::atomic<bool> closed(false);
    LibuvTestThread closer;
    if (!closer.start([&]() { gate.close(); closed.store(true); }, []() {}))
    {
        gate.release();
        return false;
    }
    const uint64_t deadline = uv_hrtime() + 5000000000ULL;
    bool rejected = false;
    while (uv_hrtime() < deadline)
    {
        if (!gate.acquire())
        {
            rejected = true;
            break;
        }
        gate.release();
        uv_sleep(0);
    }
    const bool waited = !closed.load();
    gate.release();
    const bool joined = closer.join();
    return rejected && waited && joined && closed.load() && !gate.acquire();
}

/** 连续拒绝 accept 的对象分配，恢复后同一已就绪连接仍能被接纳
@return 故障确实触发、每轮 poll 有界且恢复后可用
*/
static bool TestAcceptAllocationRecovery()
{
    LibuvTestHost server;
    LibuvTestHost client;
    uint64_t peer = 0;
    if (!server.initialize() || !client.initialize() ||
        LibuvTcp::hostListen(server.m_instance, "127.0.0.1", 0, 1) != 0 ||
        LibuvTcp::hostConnect(client.m_instance, "127.0.0.1", LibuvTcp::hostLocalPort(server.m_instance), &peer) != 0)
    {
        return false;
    }
    LibuvTcpTest::m_failAllocationSize.store(sizeof(LibuvChannelPeer));
    LibuvTcpTest::m_failAllocationCount.store(64);
    const uint64_t deadline = uv_hrtime() + 5000000000ULL;
    while (LibuvTcpTest::m_failAllocationCount.load() != 0 && uv_hrtime() < deadline)
    {
        LibuvTcp::hostPoll(server.m_instance);
        LibuvTcp::hostPoll(client.m_instance);
        uv_sleep(0);
    }
    const bool injected = LibuvTcpTest::m_failAllocationCount.load() == 0;
    LibuvTcpTest::m_failAllocationSize.store(0);
    LibuvTcpTest::m_failAllocationCount.store(0);
    bool passed = injected && PollHosts(server, client, [&]() { return server.m_connected == 1 && client.m_connected == 1; });
    passed = client.finish() && passed;
    passed = server.finish() && passed;
    return passed && client.m_closed == 1 && server.m_closed == 1;
}

#ifndef LIBUV_TEST_BASELINE
/** 拒绝写请求分配，确认已从队列取出的数组和总预算仍完整回收
@return 是否触发故障并收敛停止
*/
static bool TestWriteAllocationFailure()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    SubmitByte(tcp);
    LibuvTcpTest::m_failAllocationSize.store(sizeof(LibuvPendingWrite));
    LibuvTcpTest::m_failAllocationCount.store(1);
    LibuvBase::SendPending(tcp.m_libuv->m_asyncHandle);
    const bool passed = LibuvTcpTest::m_failAllocationCount.load() == 0 &&
        tcp.m_libuv->m_pendingBytes.load() == 0 && tcp.m_libuv->m_stopping.load();
    LibuvTcpTest::m_failAllocationSize.store(0);
    LibuvTcpTest::m_failAllocationCount.store(0);
    return passed;
}
#endif

/** 真实回环传输，逐帧检查每个生产者 FIFO 和反向接收内容
@param [in] producers 并发发送者数，1 至 16
@param [in] count 每生产者发送帧数
@param [in] payload 每帧正文大小，至少 8 字节
@param [in] stopEarly true 在真实收发中途停止，同时让生产者继续调用关闭入口
@param [in] copy 是否使用逐条借用正文重载
@param [in] mixedPost 是否循环混用send/post的数组、正文、批量及try入口
@param [in] pools 可选的初始化前配置，空使用库默认值，只借用至本函数返回
@return 是否完整接收且线程正常结束
*/
static bool TestTraffic(uint32_t producers, uint32_t count, uint32_t payload, bool stopEarly = false, bool copy = false, bool mixedPost = false, const LibuvPoolSettings* pools = nullptr)
{
    LibuvTestReceiver receiver(payload, producers);
    LibuvTestClient client;
    std::atomic<bool> start(false);
    std::atomic<bool> cancel(false);
    std::atomic<bool> stopped(false);
    std::atomic<bool> rejected(false);
    LibuvTestThread serverThread;
    LibuvTestThread clientThread;
    LibuvTestThread senders[16];
    if (pools != nullptr && (client.configureSendQueue(pools->m_sendQueue) != 0 ||
        client.configureCloseQueue(pools->m_closeQueue) != 0 ||
        client.configureConnectionPool(pools->m_connections) != 0 ||
        client.configureSendPool(pools->m_send) != 0 || client.configureReadPool(pools->m_read) != 0))
    {
        return false;
    }
    if (receiver.m_channel.listen("127.0.0.1", 0, &receiver, 1) != 0 ||
        !client.initClient("127.0.0.1", receiver.m_channel.localPort()))
    {
        return false;
    }
    const uint64_t deadline = uv_hrtime() + 20000000000ULL;
    // 功能用例限定总在途正文为 8 MiB；预算耗尽另由容量用例验证，不能要求无限接纳。
    const uint32_t window = (std::min)(uint32_t(128), (8U * 1024U * 1024U) / producers / payload);
    if (!serverThread.start([&receiver, deadline]()
    {
        bool replied = false;
        std::vector<uint8_t> reply(32768, 0xA5);
        while (!receiver.m_exit.load() && uv_hrtime() < deadline)
        {
            receiver.m_channel.poll();
            if (receiver.m_peer != 0 && !replied)
            {
                replied = receiver.m_channel.write(receiver.m_peer, &reply[0], 32768) == 0;
            }
            uv_sleep(0);
        }
        receiver.m_channel.shutdown();
    }, [&receiver]() { receiver.m_exit.store(true); }) ||
        !clientThread.start([&client]() { client.loop(); }, [&client]() { client.stop(); }))
    {
        return false;
    }
    while (client.m_target.load(std::memory_order_acquire) == nullptr && !client.m_failed.load() && uv_hrtime() < deadline)
    {
        uv_sleep(1);
    }
    bool launched = client.m_target.load() != nullptr;
    for (uint32_t producer = 0; launched && producer < producers; ++producer)
    {
        launched = senders[producer].start([&, producer]()
        {
            std::vector<char> copyBody(copy || mixedPost ? payload : 0);
            while (!start.load() && !cancel.load() && uv_hrtime() < deadline)
            {
                uv_sleep(0);
            }
            for (uint32_t sequence = 0; sequence < count && !cancel.load(); ++sequence)
            {
                // 有界在途窗口防止把正常功能验证变成内存耗尽测试。
                while (sequence >= receiver.m_counts[producer].load() + window &&
                    !cancel.load() && !stopped.load() && uv_hrtime() < deadline)
                {
                    uv_sleep(0);
                }
                if (uv_hrtime() >= deadline || receiver.m_failed.load() || cancel.load())
                {
                    break;
                }
                uv_tcp_t* target = client.m_target.load(std::memory_order_acquire);
                char* buffer = nullptr;
                const uint32_t entry = mixedPost ? sequence % 12 : (copy ? 4 : 0);
                char* body = entry >= 4 ? &copyBody[0] : nullptr;
                if (entry < 4)
                {
                    buffer = client.memoryForSend(static_cast<int32_t>(sizeof(target) + 4 + payload));
                    std::memcpy(buffer, &target, sizeof(target));
                    std::memcpy(buffer + sizeof(target), &payload, 4);
                    body = buffer + sizeof(target) + 4;
                }
                std::memcpy(body, &producer, 4);
                std::memcpy(body + 4, &sequence, 4);
                for (uint32_t offset = 8; offset < payload; ++offset)
                {
                    body[offset] = static_cast<char>(sequence + producer + offset);
                }
                bool accepted = false;
#ifndef LIBUV_TEST_BEFORE_COPY
                LibuvSendBuffer item = { body, payload };
                switch (entry)
                {
                case 0:
                {
                    accepted = client.send(buffer);
                    break;
                }
                case 1:
                {
                    accepted = client.trySend(buffer, true, true) == 0;
                    break;
                }
                case 2:
                {
                    accepted = client.post(buffer);
                    break;
                }
                case 3:
                {
                    accepted = client.tryPost(buffer, true, true) == 0;
                    break;
                }
                case 4:
                {
                    accepted = client.send(target, body, payload);
                    break;
                }
                case 5:
                {
                    accepted = client.trySend(target, body, payload, true, true) == 0;
                    break;
                }
                case 6:
                {
                    accepted = client.post(target, body, payload);
                    break;
                }
                case 7:
                {
                    accepted = client.tryPost(target, body, payload, true, true) == 0;
                    break;
                }
                case 8:
                {
                    accepted = client.sendBatch(target, &item, 1);
                    break;
                }
                case 9:
                {
                    accepted = client.trySendBatch(target, &item, 1, true, true) == 0;
                    break;
                }
                case 10:
                {
                    accepted = client.postBatch(target, &item, 1);
                    break;
                }
                default:
                {
                    accepted = client.tryPostBatch(target, &item, 1, true, true) == 0;
                    break;
                }
                }
                if (!accepted && (entry == 1 || entry == 3))
                {
                    client.releaseSendMemory(buffer);
                }
                if (entry >= 4)
                {
                    std::fill(copyBody.begin(), copyBody.end(), 'x');
                }
#else
                accepted = client.send(buffer);
#endif
                if (!accepted && !stopEarly)
                {
                    rejected.store(true);
                    break;
                }
            }
        }, [&cancel]() { cancel.store(true); });
    }
    start.store(true);
    bool complete = false;
    while (launched && !receiver.m_failed.load() && !client.m_failed.load() && uv_hrtime() < deadline)
    {
        complete = client.m_received.load() == 32768;
        for (uint32_t producer = 0; producer < producers; ++producer)
        {
            complete = complete && (stopEarly ? receiver.m_counts[producer].load() >= 16 :
                receiver.m_counts[producer].load() == count);
        }
        if (complete)
        {
            if (stopEarly)
            {
                client.stop();
                stopped.store(true);
            }
            break;
        }
        uv_sleep(1);
    }
    if (!complete || !stopEarly)
    {
        cancel.store(true);
    }
    for (uint32_t producer = 0; producer < producers; ++producer)
    {
        complete = senders[producer].join() && complete;
    }
    client.stop();
    complete = clientThread.join() && complete;
    receiver.m_exit.store(true);
    complete = serverThread.join() && complete;
    return complete && !receiver.m_failed.load() && !client.m_failed.load() && !rejected.load();
}

/** 非法初始化不能污染空对象，正常停止后允许串行重新初始化
@return 无效参数拒绝且失败、停止后的有效初始化均成功
*/
static bool TestInitializationBoundaries()
{
    LibuvTcp tcp;
    tcp.stop();
    tcp.close(nullptr);
    tcp.send(nullptr);
    if (tcp.initClient(nullptr, 1) || tcp.initClient("not-an-ip", 1) ||
        tcp.initClient("127.0.0.1", 0) || tcp.initClient("127.0.0.1", 65536) ||
        tcp.initServer(-1) || tcp.initServer(65536) || tcp.initServer(0, 0) || tcp.loopPtr() != nullptr)
    {
        return false;
    }
    for (uint32_t round = 0; round < 4; ++round)
    {
        if (!tcp.initClient("127.0.0.1", 1) || !tcp.isClient())
        {
            return false;
        }
        tcp.stop();
        tcp.loop();
        if (tcp.loopPtr() != nullptr || tcp.isClient())
        {
            return false;
        }
    }
    return true;
}

#ifndef LIBUV_TEST_BASELINE
/** 关闭先排队、数据后排队，重复关闭不能使用已经关闭的句柄或丢失数组预算
@return 旧连接的后续数据是否被完整回收
*/
static bool TestCloseBeforeData()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    uv_tcp_t* target = tcp.m_libuv->mainTcp();
    tcp.close(target);
    tcp.close(target);
    SubmitByte(tcp);
    LibuvBase::SendPending(tcp.m_libuv->m_asyncHandle);
    return tcp.m_libuv->m_pendingBytes.load() == 0 && tcp.m_libuv->m_ownerConnections.empty() &&
        uv_is_closing(reinterpret_cast<uv_handle_t*>(target));
}

/** 竞争强度超过短重试预算时仍完整接纳；资源足够，不能将 CAS 失败报为 Busy
@return 所有普通借用和预算操作均成功，最终没有残留预算
*/
static bool TestContentionDoesNotReject()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1))
    {
        return false;
    }
    LibuvLifecycleGate gate;
    gate.open();
    std::atomic<bool> start(false);
    std::atomic<bool> cancel(false);
    std::atomic<bool> failed(false);
    LibuvTestThread threads[8];
    bool launched = true;
    for (uint32_t worker = 0; launched && worker < 8; ++worker)
    {
        launched = threads[worker].start([&]()
        {
            while (!start.load() && !cancel.load())
            {
                uv_sleep(0);
            }
            for (uint32_t repeat = 0; repeat < 40000 && !cancel.load(); ++repeat)
            {
                const bool borrowed = gate.acquire();
                const bool reserved = tcp.m_libuv->tryReserveSend(64, true) == 0;
                if (!borrowed || !reserved)
                {
                    failed.store(true);
                }
                if (reserved)
                {
                    tcp.m_libuv->releaseSend(64);
                }
                if (borrowed)
                {
                    gate.release();
                }
            }
        }, [&]() { cancel.store(true); });
    }
    start.store(true);
    for (size_t worker = 0; worker < 8; ++worker)
    {
        launched = threads[worker].join() && launched;
    }
    gate.close();
    return launched && !failed.load() && tcp.m_libuv->m_pendingBytes.load() == 0;
}

/** 真实 trySend 达到预算时明确拒绝且保留调用方数组，已接纳预算在排空时归还
@return 容量拒绝与预算归还是否符合契约
*/
static bool TestActualSendCapacity()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    const int32_t size = 1024 * 1024;
    uv_tcp_t* target = tcp.m_libuv->mainTcp();
    for (uint32_t index = 0; index < 64; ++index)
    {
        char* buffer = tcp.memoryForSend(static_cast<int32_t>(sizeof(target) + 4 + size));
        std::memcpy(buffer, &target, sizeof(target));
        std::memcpy(buffer + sizeof(target), &size, sizeof(size));
        const int32_t result = tcp.trySend(buffer, true, false);
        if (result != 0)
        {
            delete[] buffer;
        }
        if ((index < 63 && result != 0) || (index == 63 && result != UV_ENOBUFS) || tcp.m_libuv->m_stopping.load())
        {
            return false;
        }
    }
    const uint32_t charge = size + static_cast<uint32_t>(sizeof(target) + 4 + sizeof(LibuvPendingWrite));
    const bool bounded = !tcp.m_libuv->m_stopping.load() && tcp.m_libuv->m_pendingBytes.load() == 63 * charge;
    tcp.stop();
    LibuvBase::SendPending(tcp.m_libuv->m_asyncHandle);
    return bounded && tcp.m_libuv->m_pendingBytes.load() == 0;
}

/** 暂停实际 TCP 读取并缩小发送缓冲，窗口满后仍可取消，或恢复后完整按序交付
@param [in] cancel true 取消连接；false 恢复读取并核对全部正文
@return 是否实际填满窗口且按选定方式正常收敛
*/
static bool TestWriteWindow(bool cancel)
{
    LibuvTestReceiver receiver(65536, 1);
    LibuvTestClient client;
    LibuvTestWindowProbe probe;
    LibuvTestThread loop;
    client.m_sendBufferSize = 1024;
    if (receiver.m_channel.listen("127.0.0.1", 0, &receiver, 1) != 0 ||
        !client.initClient("127.0.0.1", receiver.m_channel.localPort()) || !probe.start(client) ||
        !loop.start([&]() { client.loop(); }, [&]() { client.stop(); }))
    {
        // probe 句柄的寿命覆盖客户端排空，即便线程启动失败也先显式停止并驱动。
        client.stop();
        client.loop();
        return false;
    }
    const uint64_t deadline = uv_hrtime() + 15000000000ULL;
    while ((receiver.m_peer == 0 || client.m_target.load() == nullptr) && uv_hrtime() < deadline)
    {
        receiver.m_channel.poll();
        uv_sleep(0);
    }
    bool passed = receiver.m_peer != 0 && client.m_target.load() != nullptr &&
        receiver.m_channel.pauseRead(receiver.m_peer, true) == 0;
    const uint32_t payload = 65536;
    const uint32_t producer = 0;
    uv_tcp_t* target = client.m_target.load();
    for (uint32_t sequence = 0; passed && sequence < 512; ++sequence)
    {
        char* bytes = client.memoryForSend(static_cast<int32_t>(sizeof(target) + 4 + payload));
        std::memcpy(bytes, &target, sizeof(target));
        std::memcpy(bytes + sizeof(target), &payload, 4);
        char* body = bytes + sizeof(target) + 4;
        std::memcpy(body, &producer, 4);
        std::memcpy(body + 4, &sequence, 4);
        for (uint32_t offset = 8; offset < payload; ++offset)
        {
            body[offset] = static_cast<char>(sequence + offset);
        }
        client.send(bytes);
    }
    while (passed && !probe.m_full.load() && uv_hrtime() < deadline)
    {
        receiver.m_channel.poll();
        uv_sleep(1);
    }
    passed = passed && probe.m_full.load() && receiver.m_counts[0].load() == 0;
    if (cancel)
    {
        client.close(target);
        while (passed && !probe.m_canceled.load() && uv_hrtime() < deadline)
        {
            uv_sleep(1);
        }
        passed = passed && probe.m_canceled.load();
    }
    else
    {
        passed = receiver.m_channel.pauseRead(receiver.m_peer, false) == 0 && passed;
        while (passed && receiver.m_counts[0].load() != 512 && !receiver.m_failed.load() && uv_hrtime() < deadline)
        {
            receiver.m_channel.poll();
            uv_sleep(0);
        }
        passed = passed && receiver.m_counts[0].load() == 512;
    }
    client.stop();
    passed = loop.join() && passed;
    receiver.m_channel.shutdown();
    return passed && !receiver.m_failed.load() && !client.m_failed.load();
}
#endif

/** 未就绪 accept 产生的临时句柄不对外报告终态，后续真实连接仍可接纳
@return 128 次未发布接纳均回收且实际连接与数据正常
*/
static bool TestUnpublishedAccept()
{
    LibuvTestReceiver receiver(64, 1);
    LibuvChannelState state;
    LibuvTestClient client;
    LibuvTestThread loop;
    if (state.initialize() != 0 || uv_tcp_init(&state.m_loop, &state.m_listener) != 0)
    {
        return false;
    }
    state.m_listenerInitialized = true;
    state.m_listener.data = &state;
    state.m_callback = &receiver;
    sockaddr_in address = {};
    if (uv_ip4_addr("127.0.0.1", 0, &address) != 0 ||
        uv_tcp_bind(&state.m_listener, reinterpret_cast<const sockaddr*>(&address), 0) != 0 ||
        uv_listen(reinterpret_cast<uv_stream_t*>(&state.m_listener), 8, &LibuvChannelState::acceptPeer) != 0)
    {
        return false;
    }
    for (uint32_t repeat = 0; repeat < 128; ++repeat)
    {
        LibuvChannelState::acceptPeer(reinterpret_cast<uv_stream_t*>(&state.m_listener), 0);
        uv_run(&state.m_loop, UV_RUN_NOWAIT);
        if (!state.m_peers.empty() || receiver.m_peer != 0 || receiver.m_failed.load() ||
            receiver.m_connectedCount != 0 || receiver.m_closedCount != 0)
        {
            return false;
        }
    }
    int size = sizeof(address);
    if (uv_tcp_getsockname(&state.m_listener, reinterpret_cast<sockaddr*>(&address), &size) != 0 ||
        !client.initClient("127.0.0.1", ntohs(address.sin_port)) ||
        !loop.start([&]() { client.loop(); }, [&]() { client.stop(); }))
    {
        return false;
    }
    const uint64_t deadline = uv_hrtime() + 5000000000ULL;
    while ((receiver.m_peer == 0 || client.m_target.load() == nullptr) && uv_hrtime() < deadline)
    {
        uv_run(&state.m_loop, UV_RUN_NOWAIT);
        uv_sleep(0);
    }
    uv_tcp_t* target = client.m_target.load();
    bool passed = receiver.m_peer != 0 && target != nullptr;
    if (passed)
    {
        const uint32_t payload = 64;
        char* buffer = client.memoryForSend(static_cast<int32_t>(sizeof(target) + 4 + payload));
        std::memcpy(buffer, &target, sizeof(target));
        std::memcpy(buffer + sizeof(target), &payload, 4);
        char* body = buffer + sizeof(target) + 4;
        std::memset(body, 0, 8);
        for (uint32_t index = 8; index < payload; ++index)
        {
            body[index] = static_cast<char>(index);
        }
        client.send(buffer);
        while (receiver.m_counts[0].load() != 1 && uv_hrtime() < deadline)
        {
            uv_run(&state.m_loop, UV_RUN_NOWAIT);
            uv_sleep(0);
        }
        passed = receiver.m_counts[0].load() == 1;
    }
    client.stop();
    passed = loop.join() && passed;
    state.shutdown();
    uv_run(&state.m_loop, UV_RUN_DEFAULT);
    return passed && state.m_peers.empty() && receiver.m_peer == 0 && !receiver.m_failed.load() &&
        !client.m_failed.load() && receiver.m_connectedCount == 1 && receiver.m_closedCount == 1;
}

#ifndef LIBUV_TEST_BASELINE
/** 填满真实发送预算，验证普通 send 等待容量以及停止时释放借用
@param [in] cancel true 在等待时停止；false 恢复读端并完整接收
@param [in] postMode 0为send数组，1为post数组，2为post正文，3为postBatch
@return 是否实际发生容量等待，并符合恢复或取消的后置条件
*/
static bool TestSpaceWait(bool cancel, uint32_t postMode = 0)
{
    const uint32_t payload = 65536;
    const uint32_t count = 2048;
    LibuvTestReceiver receiver(payload, 1);
    LibuvTestClient client;
    LibuvTestWindowProbe probe;
    std::atomic<uint32_t> submitted(0);
    std::atomic<bool> senderDone(false);
    std::atomic<bool> senderFailed(false);
    LibuvTestThread loop;
    LibuvTestThread sender;
    client.m_sendBufferSize = 1024;
    probe.m_checkOwnerWait = true;
    if (receiver.m_channel.listen("127.0.0.1", 0, &receiver, 1) != 0 ||
        !client.initClient("127.0.0.1", receiver.m_channel.localPort()) || !probe.start(client) ||
        !loop.start([&]() { client.loop(); }, [&]() { client.stop(); }))
    {
        client.stop();
        client.loop();
        return false;
    }
    const uint64_t deadline = uv_hrtime() + 30000000000ULL;
    while ((receiver.m_peer == 0 || client.m_target.load() == nullptr) && uv_hrtime() < deadline)
    {
        receiver.m_channel.poll();
        uv_sleep(0);
    }
    if (receiver.m_peer == 0 || client.m_target.load() == nullptr ||
        receiver.m_channel.pauseRead(receiver.m_peer, true) != 0)
    {
        return false;
    }
    if (!sender.start([&]()
    {
        const uint32_t producer = 0;
        for (uint32_t sequence = 0; sequence < count; ++sequence)
        {
            uv_tcp_t* target = client.m_target.load();
            char* buffer = client.memoryForSend(static_cast<int32_t>(sizeof(target) + 4 + payload));
            std::memcpy(buffer, &target, sizeof(target));
            std::memcpy(buffer + sizeof(target), &payload, 4);
            char* body = buffer + sizeof(target) + 4;
            std::memcpy(body, &producer, 4);
            std::memcpy(body + 4, &sequence, 4);
            for (uint32_t index = 8; index < payload; ++index)
            {
                body[index] = static_cast<char>(sequence + index);
            }
            bool accepted = false;
#ifndef LIBUV_TEST_BEFORE_COPY
            if (postMode == 1)
            {
                accepted = client.post(buffer);
            }
            else if (postMode >= 2)
            {
                LibuvSendBuffer item = { body, payload };
                accepted = postMode == 2 ? client.post(target, body, payload) : client.postBatch(target, &item, 1);
                client.releaseSendMemory(buffer);
            }
            else
#endif
            {
                accepted = client.send(buffer);
            }
            if (!accepted)
            {
                senderFailed.store(true);
                break;
            }
            submitted.fetch_add(1);
        }
        senderDone.store(true);
    }, [&]() { client.stop(); }))
    {
        return false;
    }
    uint32_t observed = 0;
    uint64_t changed = uv_hrtime();
    bool waiting = false;
    while (!senderDone.load() && uv_hrtime() < deadline)
    {
        const uint32_t progress = submitted.load();
        if (progress != observed)
        {
            observed = progress;
            changed = uv_hrtime();
        }
        if (probe.m_full.load() && progress >= 512 && uv_hrtime() - changed >= 100000000ULL)
        {
            waiting = true;
            break;
        }
        receiver.m_channel.poll();
        uv_sleep(1);
    }
    bool passed = waiting && observed < count && !senderFailed.load();
    if (cancel || !passed)
    {
        client.stop();
    }
    else
    {
        passed = receiver.m_channel.pauseRead(receiver.m_peer, false) == 0;
        while (passed && receiver.m_counts[0].load() < count && !receiver.m_failed.load() && uv_hrtime() < deadline)
        {
            receiver.m_channel.poll();
            uv_sleep(0);
        }
        passed = passed && receiver.m_counts[0].load() == count;
        client.stop();
    }
    passed = sender.join() && passed;
    passed = loop.join() && passed;
    receiver.m_channel.shutdown();
    return passed && senderDone.load() && probe.m_ownerWaitChecked.load() && probe.m_ownerWaitRejected.load() &&
        !receiver.m_failed.load() && !client.m_failed.load() &&
        (cancel ? senderFailed.load() && submitted.load() < count : !senderFailed.load() && submitted.load() == count);
}

/** 固定暂停循环后填满取消队列，实际跨线程 close 必须等待而不误停实例
@return 恢复循环后关闭请求完成，实例仍可由调用方正常停止
*/
static bool TestCloseCapacity()
{
    LibuvTestReceiver receiver(64, 1);
    LibuvTestClient client;
    LibuvTestWindowProbe probe;
    std::atomic<bool> returned(false);
    std::atomic<bool> entered(false);
    std::atomic<bool> loopExited(false);
    LibuvTestThread loop;
    LibuvTestThread closer;
    if (receiver.m_channel.listen("127.0.0.1", 0, &receiver, 1) != 0 ||
        !client.initClient("127.0.0.1", receiver.m_channel.localPort()) || !probe.start(client) ||
        !loop.start([&]() { client.loop(); loopExited.store(true); }, [&]() { client.stop(); }))
    {
        client.stop();
        client.loop();
        return false;
    }
    const uint64_t deadline = uv_hrtime() + 10000000000ULL;
    while ((client.m_target.load() == nullptr || receiver.m_peer == 0) && uv_hrtime() < deadline)
    {
        receiver.m_channel.poll();
        uv_sleep(0);
    }
    uv_tcp_t* target = client.m_target.load();
    if (target == nullptr || receiver.m_peer == 0)
    {
        return false;
    }
    probe.m_pauseLoop.store(true);
    while (!probe.m_loopPaused.load() && uv_hrtime() < deadline)
    {
        uv_sleep(1);
    }
    if (!probe.m_loopPaused.load())
    {
        probe.m_pauseLoop.store(false);
        return false;
    }
    LibuvBase* state = client.m_libuv;
    LibuvPendingSend empty;
    uint32_t queued = 0;
    while (queued < 1024 && state->m_closeQueue->tryPush(empty, true, false) == LockFreeQueueSuccess)
    {
        ++queued;
    }
    // 白盒填充也必须遵循正式发布链：入队后唤醒。owner仍停在屏障，恢复后才能消费。
    LibuvBase::Wake(state->m_asyncHandle);
    if (!probe.m_loopPaused.load() || queued == 0 || queued == 1024 ||
        !closer.start([&]() { entered.store(true); client.close(target); returned.store(true); }, [&]() { client.stop(); }))
    {
        probe.m_pauseLoop.store(false);
        return false;
    }
    while (!entered.load() && uv_hrtime() < deadline)
    {
        uv_sleep(1);
    }
    uv_sleep(50);
    bool passed = entered.load() && !returned.load() && !loopExited.load();
    probe.m_pauseLoop.store(false);
    while ((!returned.load() || receiver.m_closedCount == 0) && uv_hrtime() < deadline)
    {
        receiver.m_channel.poll();
        uv_sleep(0);
    }
    // 后续观察来自owner发布的计数，不再解引用可能因错误实现提前回收的state。
    const uint32_t ticks = probe.m_ticks.load();
    while (!loopExited.load() && probe.m_ticks.load() - ticks < 3 && uv_hrtime() < deadline)
    {
        uv_sleep(1);
    }
    passed = passed && returned.load() && receiver.m_closedCount == 1 &&
        !loopExited.load() && probe.m_ticks.load() - ticks >= 3;
    client.stop();
    passed = closer.join() && passed;
    passed = loop.join() && passed;
    receiver.m_channel.shutdown();
    return passed && !receiver.m_failed.load();
}

/** 在满预算下分别选择两项策略，try失败不能接管数组或误归还其他请求预算
@return 四种组合、空输入、预算返回值及退避计数边界是否正确
*/
static bool TestTryOptions()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    LibuvBase* state = tcp.m_libuv;
    const uint32_t limit = 64U * 1024U * 1024U;
    if (state->tryReserveSend(limit, true) != 0)
    {
        return false;
    }
    bool passed = tcp.trySend(nullptr) == UV_EINVAL;
    const int32_t length = 1;
    uv_tcp_t* target = state->mainTcp();
    for (uint32_t flags = 0; flags < 4; ++flags)
    {
        char* buffer = tcp.memoryForSend(static_cast<int32_t>(sizeof(target) + 5));
        std::memcpy(buffer, &target, sizeof(target));
        std::memcpy(buffer + sizeof(target), &length, 4);
        buffer[sizeof(target) + 4] = 'x';
        LibuvTcpTest::m_arrayDeletes.store(0);
        LibuvTcpTest::m_watchedArray.store(buffer);
        const int32_t result = tcp.trySend(buffer, (flags & 1) != 0, (flags & 2) != 0);
        // 此处尚未运行 loop，等待它释放空间属于非法上下文，不能无限等待。
        passed = result == ((flags & 2) != 0 ? UV_EPERM : UV_ENOBUFS) &&
            LibuvTcpTest::m_arrayDeletes.load() == 0 && state->m_pendingBytes.load() == limit && passed;
        tcp.releaseSendMemory(buffer);
        passed = LibuvTcpTest::m_arrayDeletes.load() == 1 && passed;
        LibuvTcpTest::m_watchedArray.store(nullptr);
    }
    tcp.releaseSendMemory(nullptr);
    state->releaseSend(limit);
    passed = state->tryReserveSend(0, false) == UV_EINVAL &&
        state->tryReserveSend(0xffffffffU, false) == UV_EINVAL && passed;
    uint32_t attempts = 0;
    for (uint32_t index = 1; index <= 4098; ++index)
    {
        LibuvBase::WaitForSpace(attempts);
        passed = attempts == (index < 4096 ? index : 4096) && passed;
    }
    return passed && state->m_pendingBytes.load() == 0 && !state->m_stopping.load();
}
#endif

#ifndef LIBUV_TEST_BEFORE_BATCH
/** 观察一批独立复制的线上字节，再按实际所有权释放，不驱动未连接TCP
@param [in,out] tcp 本次测试实例
@param [in] expected 期望的完整线上字节
@return 正文、长度、预算和原数组是否相互独立
*/
static bool InspectBatch(LibuvTcp& tcp, const std::vector<char>& expected)
{
    LibuvPendingSend pending;
    if (!PopSubmitted(tcp.m_libuv, &pending))
    {
        return false;
    }
#ifndef LIBUV_TEST_BEFORE_COPY
    const char* bytes = pending.m_buffer == nullptr ? pending.m_inlineBytes : pending.m_buffer + sizeof(uv_tcp_t*);
#else
    const char* bytes = pending.m_buffer + sizeof(uv_tcp_t*);
#endif
    const bool passed = pending.m_wireSize == expected.size() &&
        std::memcmp(bytes, &expected[0], expected.size()) == 0;
    tcp.releaseSendMemory(pending.m_buffer);
    tcp.m_libuv->releaseSend(pending.m_charge);
    return passed && tcp.m_libuv->m_pendingBytes.load() == 0;
}

/** 参数验证不能越界读无效输入；成功接纳后调用方可立即修改原正文
@return 空、极值、4096项和1MiB线上容量边界是否正确
*/
static bool TestBatchContract()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    uv_tcp_t* target = tcp.m_libuv->mainTcp();
    char data[] = "123456";
    LibuvSendBuffer buffers[] = { { data, 6 }, { nullptr, 0 }, { data, 2 } };
    bool passed = tcp.trySendBatch(nullptr, buffers, 1) == UV_EINVAL &&
        tcp.trySendBatch(target, nullptr, 1) == UV_EINVAL && tcp.trySendBatch(target, buffers, 0) == UV_EINVAL &&
        tcp.trySendBatch(target, buffers, 4097) == UV_EINVAL;
    LibuvSendBuffer invalid = { nullptr, 1 };
    passed = tcp.trySendBatch(target, &invalid, 1) == UV_EINVAL && passed;
    invalid.m_data = data;
    invalid.m_size = 0xffffffffU;
    passed = tcp.trySendBatch(target, &invalid, 1) == UV_EINVAL && passed;
    std::vector<char> expected(20, 0);
    const uint32_t six = 6;
    const uint32_t two = 2;
    std::memcpy(&expected[0], &six, 4);
    std::memcpy(&expected[4], data, 6);
    std::memcpy(&expected[14], &two, 4);
    std::memcpy(&expected[18], data, 2);
    passed = tcp.sendBatch(target, buffers, 3) && passed;
    std::memset(data, 'z', 6);
    passed = InspectBatch(tcp, expected) && passed;
    LibuvSendBuffer empty = { nullptr, 0 };
    std::vector<LibuvSendBuffer> maximum(4096, empty);
    passed = tcp.sendBatch(target, &maximum[0], 4096) && passed;
    passed = InspectBatch(tcp, std::vector<char>(16384, 0)) && passed;
    std::vector<char> large(1048572, 'x');
    LibuvSendBuffer limit = { &large[0], static_cast<uint32_t>(large.size()) };
    passed = tcp.sendBatch(target, &limit, 1) && passed;
    expected.resize(1048576);
    std::memcpy(&expected[0], &limit.m_size, 4);
    std::memcpy(&expected[4], &large[0], large.size());
    passed = InspectBatch(tcp, expected) && passed;
    ++limit.m_size;
    passed = tcp.trySendBatch(target, &limit, 1) == UV_EINVAL && passed;
    return passed && !tcp.m_libuv->sendsReady();
}

/** 整批在参数、分配、容量失败时不能部分发布或误归还其他请求预算
@return 四种等待策略、分配故障及停止返回是否正确
*/
static bool TestBatchFailures()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    uv_tcp_t* target = tcp.m_libuv->mainTcp();
    LibuvSendBuffer body = { "123456", 6 };
    LibuvTcpTest::m_failAllocationSize.store(sizeof(target) + 10);
    LibuvTcpTest::m_failAllocationCount.store(1);
    bool passed = tcp.trySendBatch(target, &body, 1) == UV_ENOMEM &&
        LibuvTcpTest::m_failAllocationCount.load() == 0;
    LibuvTcpTest::m_failAllocationSize.store(0);
    const uint32_t limit = 64U * 1024U * 1024U;
    passed = tcp.m_libuv->tryReserveSend(limit, true) == 0 && passed;
    for (uint32_t flags = 0; flags < 4; ++flags)
    {
        const int32_t result = tcp.trySendBatch(target, &body, 1, (flags & 1) != 0, (flags & 2) != 0);
        passed = result == ((flags & 2) != 0 ? UV_EPERM : UV_ENOBUFS) && passed;
        passed = tcp.m_libuv->m_pendingBytes.load() == limit && !tcp.m_libuv->sendsReady() && passed;
    }
    tcp.m_libuv->releaseSend(limit);
    tcp.stop();
    return tcp.trySendBatch(target, &body, 1) == UV_ECANCELED && passed;
}

/** 辅助合批分配失败时回退逐帧，旧代数据仍必须恰好释放并归还所有预算
@return 确实注入故障且没有因辅助分配失败停止实例
*/
static bool TestMergeAllocation()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    for (size_t index = 0; index < 200; ++index)
    {
        SubmitByte(tcp);
    }
    if (!tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    LibuvTcpTest::m_failAllocationSize.store(sizeof(uv_tcp_t*) + 1000);
    LibuvTcpTest::m_failAllocationCount.store(1);
    LibuvBase::SendPending(tcp.m_libuv->m_asyncHandle);
    const bool passed = LibuvTcpTest::m_failAllocationCount.load() == 0 &&
        tcp.m_libuv->m_pendingBytes.load() == 0 && !tcp.m_libuv->m_stopping.load();
    LibuvTcpTest::m_failAllocationSize.store(0);
    LibuvTcpTest::m_failAllocationCount.store(0);
    return passed;
}

/** 停止包含多帧的队列操作，核对数组所有权和累计预算的回收
@return 是否没有剩余预算或错误重复释放
*/
static bool TestBatchStopped()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    LibuvSendBuffer body = { "123456", 6 };
    std::vector<LibuvSendBuffer> buffers(4096, body);
    if (!tcp.sendBatch(tcp.m_libuv->mainTcp(), &buffers[0], 4096))
    {
        return false;
    }
    tcp.stop();
    LibuvBase::SendPending(tcp.m_libuv->m_asyncHandle);
    return tcp.m_libuv->m_pendingBytes.load() == 0 && !tcp.m_libuv->sendsReady() &&
        tcp.m_libuv->m_activeWrites == 0;
}

/** 六种批量大小都包含不整除的尾批，分别核对四条生产者的独立序号
@return 全部真实接收并验证，不允许缺帧或重排
*/
static bool TestBatchOrder()
{
    const uint32_t sizes[] = { 2, 16, 64, 256, 1024, 4096 };
    for (size_t index = 0; index < sizeof(sizes) / sizeof(sizes[0]); ++index)
    {
        if (LibuvTcpBenchmark::tiny(4, 1, 100003, sizes[index]) != 0)
        {
            return false;
        }
    }
    return true;
}
#endif

#ifndef LIBUV_TEST_BEFORE_COPY
/** 复制入口覆盖零长、内联临界和大帧，接纳后源正文可立即覆盖
@return 参数错误不发布，成功时独立字节与长度均保持不变
*/
static bool TestCopyContract()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    uv_tcp_t* target = tcp.m_libuv->mainTcp();
    bool passed = tcp.trySend(nullptr, "x", 1) == UV_EINVAL &&
        tcp.trySend(target, nullptr, 1) == UV_EINVAL &&
        tcp.trySend(target, "x", 0xffffffffU) == UV_EINVAL;
    const uint32_t sizes[] = {0, 1, 6, 12, 13, 1024, 1048576};
    for (size_t index = 0; index < sizeof(sizes) / sizeof(sizes[0]); ++index)
    {
        const uint32_t size = sizes[index];
        std::vector<char> source(size, 'v');
        std::vector<char> expected(size + 4, 'v');
        std::memcpy(&expected[0], &size, 4);
        if (tcp.trySend(target, size ? &source[0] : nullptr, size, true, false) != 0)
        {
            return false;
        }
        std::fill(source.begin(), source.end(), 'x');
        passed = InspectBatch(tcp, expected) && passed;
    }
    tcp.stop();
    return passed && tcp.trySend(target, "x", 1) == UV_ECANCELED && !tcp.m_libuv->sendsReady();
}

/** 故障注入只存在测试工程，内联正文不申请独立数组，失败仍无发布
@return 分配失败、满预算的四种重试策略与返回码是否正确
*/
static bool TestCopyFailures()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    LibuvBase* state = tcp.m_libuv;
    uv_tcp_t* target = state->mainTcp();
    LibuvTcpTest::m_failAllocationSize.store(sizeof(target) + 10);
    LibuvTcpTest::m_failAllocationCount.store(1);
    bool passed = tcp.send(target, "123456", 6) && LibuvTcpTest::m_failAllocationCount.load() == 1;
    LibuvTcpTest::m_failAllocationCount.store(0);
    std::vector<char> expected(10);
    uint32_t length = 6;
    std::memcpy(&expected[0], &length, 4);
    std::memcpy(&expected[4], "123456", 6);
    passed = InspectBatch(tcp, expected) && passed;
    LibuvTcpTest::m_failAllocationSize.store(sizeof(target) + 17);
    LibuvTcpTest::m_failAllocationCount.store(1);
    passed = tcp.trySend(target, "1234567890123", 13) == UV_ENOMEM && passed;
    LibuvTcpTest::m_failAllocationCount.store(0);
    const uint32_t limit = 64U * 1024U * 1024U;
    if (state->tryReserveSend(limit, true) != 0)
    {
        return false;
    }
    for (uint32_t flags = 0; flags < 4; ++flags)
    {
        const bool wait = (flags & 2U) != 0;
        passed = tcp.trySend(target, "x", 1, (flags & 1U) != 0, wait) ==
            (wait ? UV_EPERM : UV_ENOBUFS) && passed;
    }
    state->releaseSend(limit);
    return passed && state->m_pendingBytes.load() == 0 && !state->sendsReady();
}

/** 三种post输入边界，普通和try正文返回后原数据可复用，批量保持原帧格式
@return 零长、内联边界、1MiB、4096帧及越界全部符合契约
*/
static bool TestPostContract()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    uv_tcp_t* target = tcp.m_libuv->mainTcp();
    bool passed = !tcp.post(nullptr) && tcp.tryPost(nullptr) == UV_EINVAL &&
        tcp.tryPost(nullptr, "x", 1) == UV_EINVAL && tcp.tryPost(target, nullptr, 1) == UV_EINVAL &&
        tcp.tryPost(target, "x", 0xffffffffU) == UV_EINVAL;
    const uint32_t sizes[] = { 0, 1, 6, 12, 13, 1024, 1048576 };
    for (size_t index = 0; index < sizeof(sizes) / sizeof(sizes[0]); ++index)
    {
        const uint32_t size = sizes[index];
        for (uint32_t mode = 0; mode < 2; ++mode)
        {
            std::vector<char> source(size, 'v');
            std::vector<char> expected(size + 4, 'v');
            std::memcpy(&expected[0], &size, 4);
            const char* data = size ? &source[0] : nullptr;
            const bool accepted = mode == 0 ? tcp.post(target, data, size) : tcp.tryPost(target, data, size) == 0;
            if (!accepted)
            {
                return false;
            }
            std::fill(source.begin(), source.end(), 'x');
            passed = InspectBatch(tcp, expected) && passed;
        }
    }
    char data[] = "123456";
    LibuvSendBuffer items[] = { { data, 6 }, { nullptr, 0 }, { data, 2 } };
    passed = tcp.tryPostBatch(nullptr, items, 1) == UV_EINVAL &&
        tcp.tryPostBatch(target, nullptr, 1) == UV_EINVAL && tcp.tryPostBatch(target, items, 0) == UV_EINVAL &&
        tcp.tryPostBatch(target, items, 4097) == UV_EINVAL && passed;
    LibuvSendBuffer invalid = { nullptr, 1 };
    passed = tcp.tryPostBatch(target, &invalid, 1) == UV_EINVAL && passed;
    invalid.m_data = data;
    invalid.m_size = 0xffffffffU;
    passed = tcp.tryPostBatch(target, &invalid, 1) == UV_EINVAL && passed;
    std::vector<char> expected(20, 0);
    const uint32_t six = 6;
    const uint32_t two = 2;
    std::memcpy(&expected[0], &six, 4);
    std::memcpy(&expected[4], data, 6);
    std::memcpy(&expected[14], &two, 4);
    std::memcpy(&expected[18], data, 2);
    passed = tcp.postBatch(target, items, 3) && passed;
    std::memset(data, 'z', 6);
    passed = InspectBatch(tcp, expected) && passed;
    LibuvSendBuffer empty = { nullptr, 0 };
    std::vector<LibuvSendBuffer> maximum(4096, empty);
    passed = tcp.tryPostBatch(target, &maximum[0], 4096) == 0 && passed;
    passed = InspectBatch(tcp, std::vector<char>(16384, 0)) && passed;
    std::vector<char> large(1048572, 'x');
    LibuvSendBuffer limit = { &large[0], 1048572 };
    passed = tcp.postBatch(target, &limit, 1) && passed;
    expected.assign(1048576, 'x');
    std::memcpy(&expected[0], &limit.m_size, 4);
    passed = InspectBatch(tcp, expected) && passed;
    ++limit.m_size;
    passed = tcp.tryPostBatch(target, &limit, 1) == UV_EINVAL && passed;
    tcp.stop();
    return passed && tcp.tryPost(target, "x", 1) == UV_ECANCELED &&
        tcp.tryPostBatch(target, items, 1) == UV_ECANCELED && !tcp.m_libuv->sendsReady();
}

/** 满预算四组策略验证数组保留，普通post失败释放；分配失败不得接纳或泄漏预算
@return 三种输入均正确区分容量和非法等待，数组在正确拥有者处仅释放一次
*/
static bool TestPostFailures()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    LibuvBase* state = tcp.m_libuv;
    uv_tcp_t* target = state->mainTcp();
    LibuvSendBuffer item = { "1234567890123", 13 };
    LibuvTcpTest::m_failAllocationSize.store(sizeof(target) + 17);
    LibuvTcpTest::m_failAllocationCount.store(1);
    bool passed = tcp.tryPost(target, item.m_data, item.m_size) == UV_ENOMEM &&
        LibuvTcpTest::m_failAllocationCount.load() == 0;
    LibuvTcpTest::m_failAllocationCount.store(1);
    passed = tcp.tryPostBatch(target, &item, 1) == UV_ENOMEM &&
        LibuvTcpTest::m_failAllocationCount.load() == 0 && passed;
    LibuvTcpTest::m_failAllocationCount.store(0);
    LibuvTcpTest::m_failAllocationSize.store(0);
    const uint32_t limit = 64U * 1024U * 1024U;
    if (state->tryReserveSend(limit, true) != 0)
    {
        return false;
    }
    const int32_t length = 13;
    for (uint32_t flags = 0; flags < 4; ++flags)
    {
        char* buffer = tcp.memoryForSend(static_cast<int32_t>(sizeof(target) + 4 + length));
        std::memcpy(buffer, &target, sizeof(target));
        std::memcpy(buffer + sizeof(target), &length, 4);
        std::memcpy(buffer + sizeof(target) + 4, item.m_data, length);
        LibuvTcpTest::m_arrayDeletes.store(0);
        LibuvTcpTest::m_watchedArray.store(buffer);
        const bool retry = (flags & 1) != 0;
        const bool wait = (flags & 2) != 0;
        const int32_t expected = wait ? UV_EPERM : UV_ENOBUFS;
        passed = tcp.tryPost(buffer, retry, wait) == expected &&
            LibuvTcpTest::m_arrayDeletes.load() == 0 && passed;
        passed = tcp.tryPost(target, item.m_data, item.m_size, retry, wait) == expected &&
            tcp.tryPostBatch(target, &item, 1, retry, wait) == expected && passed;
        passed = state->m_pendingBytes.load() == limit && !state->sendsReady() && passed;
        // 未启动loop且预算已满，普通post不能等待自身；失败仍须接管释放。
        passed = !tcp.post(buffer) && LibuvTcpTest::m_arrayDeletes.load() == 1 && passed;
        LibuvTcpTest::m_watchedArray.store(nullptr);
    }
    state->releaseSend(limit);
    // 两侧覆盖内联复制立即释放以及大帧延迟到消费释放。
    const uint32_t sizes[] = { 6, 13 };
    for (uint32_t mode = 0; mode < 2; ++mode)
    {
        for (size_t index = 0; index < 2; ++index)
        {
            const uint32_t size = sizes[index];
            char* buffer = tcp.memoryForSend(static_cast<int32_t>(sizeof(target) + 4 + size));
            std::memcpy(buffer, &target, sizeof(target));
            std::memcpy(buffer + sizeof(target), &size, 4);
            std::memcpy(buffer + sizeof(target) + 4, item.m_data, size);
            LibuvTcpTest::m_arrayDeletes.store(0);
            LibuvTcpTest::m_watchedArray.store(buffer);
            const bool accepted = mode == 0 ? tcp.post(buffer) : tcp.tryPost(buffer) == 0;
            LibuvPendingSend pending;
            const bool popped = PopSubmitted(state, &pending);
            passed = accepted && popped && LibuvTcpTest::m_arrayDeletes.load() == (size <= 12 ? 1 : 0) && passed;
            if (popped)
            {
                tcp.releaseSendMemory(pending.m_buffer);
                state->releaseSend(pending.m_charge);
            }
            passed = LibuvTcpTest::m_arrayDeletes.load() == 1 && passed;
            LibuvTcpTest::m_watchedArray.store(nullptr);
        }
    }
    return passed && state->m_pendingBytes.load() == 0 && !state->sendsReady();
}

/** 各种大小混用全部十二个提交入口，接收方逐条校验正文及单生产者序号
@return 内联临界、大帧、双向字节与并发停止全部正确
*/
static bool TestPostSizes()
{
    const uint32_t sizes[] = { 8, 12, 13, 64, 1024, 65536 };
    for (size_t index = 0; index < sizeof(sizes) / sizeof(sizes[0]); ++index)
    {
        if (!TestTraffic(4, 257, sizes[index], false, false, true))
        {
            return false;
        }
    }
    return TestTraffic(4, 5000, 64, true, false, true);
}

/** 三种普通post填满64MiB后，分别验证恢复交付或停止解除等待
@param [in] cancel true停止解阻，false恢复接收并校验128MiB
@return 所有入口符合等待、所有权和唯一loop资源释放约定
*/
static bool TestPostWait(bool cancel)
{
    for (uint32_t mode = 1; mode <= 3; ++mode)
    {
        if (!TestSpaceWait(cancel, mode))
        {
            return false;
        }
    }
    return true;
}

#ifndef LIBUV_TEST_BEFORE_10M
/** 同线程缓存重入及槽位冲突不等待拥有者，退回原队列后仍按实际副本交付
@return 缓存正常、重入回退、冲突回退均完整提交且预算归零
*/
static bool TestProducerFallback()
{
    LibuvTcp tcp;
    LibuvTcp collision;
    LibuvSendPoolConfig send;
    send.m_laneCapacity = 0;
    if (tcp.configureSendPool(send) != 0 || collision.configureSendPool(send) != 0)
    {
        return false;
    }
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()) ||
        !collision.initClient("127.0.0.1", 1) || !collision.m_libuv->registerTcp(collision.m_libuv->mainTcp()))
    {
        return false;
    }
    const uint32_t token = LibuvBase::producerToken();
    if (token == 0 || token == UINT32_MAX)
    {
        return false;
    }
    // 本例专门覆盖并发队列的缓存与回退；单写通道由独立用例覆盖。
    tcp.m_libuv->m_sendLane.m_token.store(UINT32_MAX);
    collision.m_libuv->m_sendLane.m_token.store(UINT32_MAX);
    LibuvSendProducer& slot = tcp.m_libuv->m_sendProducers[token % 16];
    std::vector<char> expected(10);
    const uint32_t size = 6;
    std::memcpy(&expected[0], &size, 4);
    std::memcpy(&expected[4], "123456", 6);
    bool passed = tcp.send(tcp.m_libuv->mainTcp(), "123456", 6) &&
        slot.m_token.load() == token && slot.m_producer.attached();
    passed = InspectBatch(tcp, expected) && passed;
    slot.m_busy = true;
    passed = tcp.post(tcp.m_libuv->mainTcp(), "123456", 6) && passed;
    passed = InspectBatch(tcp, expected) && slot.m_busy && passed;
    slot.m_busy = false;
    passed = tcp.tryPost(tcp.m_libuv->mainTcp(), "123456", 6, true, false) == 0 && passed;
    passed = InspectBatch(tcp, expected) && slot.m_producer.attached() && passed;
    // 白盒占据一个槽，但不构造其他线程的上下文；本线程必须只用普通入口。
    LibuvSendProducer& occupied = collision.m_libuv->m_sendProducers[token % 16];
    occupied.m_token.store(UINT32_MAX);
    passed = collision.send(collision.m_libuv->mainTcp(), "123456", 6) && passed;
    return InspectBatch(collision, expected) && !occupied.m_producer.attached() && passed;
}

/** 四十个同时存在的线程在同一实例提交，覆盖缓存16槽之外的普通路径
@return 令牌互不相同，每线程128条序号完整，消费和静止回收不依赖已退出线程
*/
static bool TestProducerConcurrent()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    uv_tcp_t* target = tcp.m_libuv->mainTcp();
    const uint32_t producers = 40;
    const uint32_t count = 128;
    uint32_t tokens[40] = {};
    uint32_t received[40] = {};
    std::atomic<uint32_t> ready(0);
    std::atomic<uint32_t> finished(0);
    std::atomic<bool> start(false);
    std::atomic<bool> failed(false);
    LibuvTestThread threads[40];
    const uint64_t deadline = uv_hrtime() + 10000000000ULL;
    // 白盒消费者在本线程接管队列排空与预算归还，必须与生产并行，不能假设物理池只计消息数。
    // 这里不运行真实网络回调；仅替代loop的消费者职责，使跨线程普通send/post可以等待容量。
    tcp.m_libuv->m_ownerThread = uv_thread_self();
    tcp.m_libuv->m_ownerReady.store(true, std::memory_order_release);
    bool passed = true;
    for (uint32_t producer = 0; producer < producers; ++producer)
    {
        // 每个任务只写自己的令牌格；join之后才由测试线程检查，barrier确保线程真实并存。
        if (!threads[producer].start([&, producer]()
        {
            tokens[producer] = LibuvBase::producerToken();
            ready.fetch_add(1);
            while (!start.load() && uv_hrtime() < deadline)
            {
                uv_sleep(1);
            }
            if (!start.load())
            {
                failed.store(true);
                finished.fetch_add(1);
                return;
            }
            for (uint32_t sequence = 0; sequence < count; ++sequence)
            {
                char body[8];
                std::memcpy(body, &producer, 4);
                std::memcpy(body + 4, &sequence, 4);
                if (!(sequence & 1 ? tcp.send(target, body, 8) : tcp.post(target, body, 8)))
                {
                    failed.store(true);
                    break;
                }
            }
            finished.fetch_add(1);
        }, [&]() { start.store(true); tcp.stop(); }))
        {
            passed = false;
            break;
        }
    }
    while (passed && ready.load() != producers && uv_hrtime() < deadline)
    {
        uv_sleep(1);
    }
    passed = passed && ready.load() == producers;
    start.store(true);
    LibuvPendingSend pending;
    uint32_t total = 0;
    while (passed && total != producers * count && uv_hrtime() < deadline)
    {
        if (!PopSubmitted(tcp.m_libuv, &pending))
        {
            if (finished.load() == producers)
            {
                // 完成发布可能发生在上一次空观察之后；取得全部完成标志后再确认一次尾部。
                if (!PopSubmitted(tcp.m_libuv, &pending))
                {
                    break;
                }
            }
            else
            {
                uv_sleep(0);
                continue;
            }
        }
        uint32_t size = 0;
        uint32_t producer = 0;
        uint32_t sequence = 0;
        std::memcpy(&size, pending.m_inlineBytes, 4);
        std::memcpy(&producer, pending.m_inlineBytes + 4, 4);
        std::memcpy(&sequence, pending.m_inlineBytes + 8, 4);
        passed = pending.m_buffer == nullptr && pending.m_wireSize == 12 && size == 8 &&
            producer < producers && sequence == received[producer] && passed;
        if (producer < producers)
        {
            ++received[producer];
        }
        ++total;
        tcp.releaseSendMemory(pending.m_buffer);
        tcp.m_libuv->releaseSend(pending.m_charge);
    }
    if (!passed || total != producers * count)
    {
        tcp.stop();
    }
    for (uint32_t producer = 0; producer < producers; ++producer)
    {
        passed = threads[producer].join() && passed;
        passed = tokens[producer] != 0 && tokens[producer] != UINT32_MAX && passed;
        for (uint32_t earlier = 0; earlier < producer; ++earlier)
        {
            passed = tokens[earlier] != tokens[producer] && passed;
        }
        passed = received[producer] == count && passed;
    }
    tcp.m_libuv->m_ownerReady.store(false, std::memory_order_release);
    if (!passed || failed.load() || total != producers * count || tcp.m_libuv->m_pendingBytes.load() != 0)
    {
        std::printf("producer-summary ready=%u failed=%u total=%u pending=%llu passed=%u\n",
            ready.load(), failed.load() ? 1U : 0U, total, static_cast<unsigned long long>(tcp.m_libuv->m_pendingBytes.load()), passed ? 1U : 0U);
        for (uint32_t producer = 0; producer < producers; ++producer)
        {
            std::printf("producer=%u token=%u received=%u\n", producer, tokens[producer], received[producer]);
        }
    }
    return passed && !failed.load() && total == producers * count && tcp.m_libuv->m_pendingBytes.load() == 0;
}

/** 同一外层对象反复停止重建，生产者线程令牌保留但队列登记必须随旧实例释放
@return 每轮能重新接纳，loop返回后状态已销毁，不借用旧队列或析构后TLS对象
*/
static bool TestProducerLifecycle()
{
    LibuvTcp tcp;
    for (uint32_t repeat = 0; repeat < 128; ++repeat)
    {
        if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()) ||
            !tcp.post(tcp.m_libuv->mainTcp(), "123456", 6))
        {
            return false;
        }
        tcp.stop();
        tcp.loop();
        if (tcp.m_libuv != nullptr)
        {
            return false;
        }
    }
    return true;
}

/** 真正填满队列后同时阻塞16个提交者，验证恢复和停止都不依赖生产者独占执行
@param [in] stop true在容量恢复前停止，false由测试线程接管消费者并逐项验证
@return 全部提交者按契约完成，逐线程有序且预算最终归零
*/
static bool TestCapacityConcurrent(bool stop)
{
    LibuvTcp tcp;
    if (!FillQueue(tcp))
    {
        return false;
    }
    LibuvBase* state = tcp.m_libuv;
    uv_tcp_t* target = state->mainTcp();
    state->m_ownerThread = uv_thread_self();
    state->m_ownerReady.store(true, std::memory_order_release);
    std::atomic<uint32_t> entered(0);
    std::atomic<uint32_t> finished(0);
    std::atomic<bool> failed(false);
    uint32_t received[16] = {};
    LibuvTestThread threads[16];
    const uint64_t deadline = uv_hrtime() + 10000000000ULL;
    bool passed = true;
    for (uint32_t producer = 0; producer < 16; ++producer)
    {
        // 每个任务串行生成自己的序号；所有提交接口共享同一个满队列及生命周期。
        if (!threads[producer].start([&, producer]()
        {
            entered.fetch_add(1);
            for (uint32_t sequence = 0; sequence < 128; ++sequence)
            {
                char body[8];
                std::memcpy(body, &producer, 4);
                std::memcpy(body + 4, &sequence, 4);
                int32_t result = 0;
                switch (producer & 3U)
                {
                case 0:
                    result = tcp.send(target, body, 8) ? 0 : UV_ECANCELED;
                    break;
                case 1:
                    result = tcp.post(target, body, 8) ? 0 : UV_ECANCELED;
                    break;
                case 2:
                    result = tcp.trySend(target, body, 8, true, true);
                    break;
                default:
                    result = tcp.tryPost(target, body, 8, true, true);
                    break;
                }
                if (stop || result != 0)
                {
                    if (!stop || result != UV_ECANCELED || sequence != 0)
                    {
                        failed.store(true);
                    }
                    break;
                }
            }
            finished.fetch_add(1);
        }, [&]() { tcp.stop(); }))
        {
            passed = false;
            break;
        }
    }
    while (passed && entered.load() != 16 && uv_hrtime() < deadline)
    {
        uv_sleep(1);
    }
    uv_sleep(50);
    passed = passed && entered.load() == 16 && finished.load() == 0;
    if (stop || !passed)
    {
        tcp.stop();
    }
    uint32_t total = 0;
    LibuvPendingSend pending;
    while (passed && !stop && total != 16 * 128 && uv_hrtime() < deadline)
    {
        if (!PopSubmitted(state, &pending))
        {
            uv_sleep(0);
            continue;
        }
        if (pending.m_charge != 0)
        {
            uint32_t producer = 0;
            uint32_t sequence = 0;
            std::memcpy(&producer, pending.m_inlineBytes + 4, 4);
            std::memcpy(&sequence, pending.m_inlineBytes + 8, 4);
            passed = pending.m_buffer == nullptr && pending.m_wireSize == 12 &&
                producer < 16 && sequence == received[producer] && passed;
            if (producer < 16)
            {
                ++received[producer];
            }
            ++total;
            state->releaseSend(pending.m_charge);
            tcp.releaseSendMemory(pending.m_buffer);
        }
    }
    if (!stop && total != 16 * 128)
    {
        passed = false;
        tcp.stop();
    }
    for (uint32_t producer = 0; producer < 16; ++producer)
    {
        passed = threads[producer].join() && passed;
        passed = received[producer] == (stop ? 0U : 128U) && passed;
    }
    state->m_ownerReady.store(false, std::memory_order_release);
    return passed && !failed.load() && finished.load() == 16 && state->m_pendingBytes.load() == 0;
}
#endif

/** 索引白盒使用地址数值作键，不将构造的键交给libuv或解引用
@return 直接映射缓存碰撞不得串线，同一真实地址换代后必须返回新代次
*/
static bool TestConnectionCache()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1))
    {
        return false;
    }
    LibuvBase* state = tcp.m_libuv;
    for (uintptr_t index = 1; index <= 32; ++index)
    {
        uv_tcp_t* key = reinterpret_cast<uv_tcp_t*>(index * 1024);
        if (!state->registerTcp(key))
        {
            return false;
        }
    }
    for (uintptr_t index = 1; index <= 32; ++index)
    {
        uint64_t generation = 0;
        if (!state->connectionGeneration(reinterpret_cast<uv_tcp_t*>(index * 1024), generation) || generation != index)
        {
            return false;
        }
    }
    for (uint32_t index = 0; index < 2048; ++index)
    {
        if (!state->registerTcp(state->mainTcp()))
        {
            return false;
        }
        uint64_t generation = 0;
        if (!state->connectionGeneration(state->mainTcp(), generation) || generation != 33U + index)
        {
            return false;
        }
        state->m_connections.maintain(16);
    }
    state->closeTcp(state->mainTcp());
    uint64_t absent = 0;
    return !state->connectionGeneration(state->mainTcp(), absent);
}

/** 单个内联帧无法申请写请求时，必须归还预算并请求停止，不能借用已退出的栈
@return 无残余在途写或预算且停止状态可观察
*/
static bool TestInlineWriteFailure()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1) || !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()) ||
        !tcp.send(tcp.m_libuv->mainTcp(), "123456", 6))
    {
        return false;
    }
    LibuvTcpTest::m_failAllocationSize.store(sizeof(LibuvPendingWrite));
    LibuvTcpTest::m_failAllocationCount.store(1);
    LibuvBase::SendPending(tcp.m_libuv->m_asyncHandle);
    const bool failed = LibuvTcpTest::m_failAllocationCount.load() == 0;
    LibuvTcpTest::m_failAllocationCount.store(0);
    return failed && tcp.m_libuv->m_pendingBytes.load() == 0 && tcp.m_libuv->m_stopping.load() &&
        tcp.m_libuv->m_activeWrites == 0;
}

/** 真实TCP验证内联上下界和大帧复制，不依赖队列布局判定交付
@return 正文、顺序、反向数据及线程退出是否全部正确
*/
static bool TestCopySizes()
{
    const uint32_t sizes[] = {8, 12, 13, 64, 1024, 65536};
    for (size_t index = 0; index < sizeof(sizes) / sizeof(sizes[0]); ++index)
    {
        if (!TestTraffic(4, 257, sizes[index], false, true))
        {
            return false;
        }
    }
    return TestTraffic(4, 5000, 64, true, true);
}
#endif

/** 固定容量通道跨uint32回绕反复读写，并逐字节检查0至16字节复制不越界
@return 满容量不接纳、释放后恢复、顺序和两侧哨兵均正确
*/
static bool TestLaneBoundaries()
{
    LibuvSendLane lane;
    const uint32_t start = UINT32_MAX - 511;
    lane.m_write = start;
    lane.m_seenRead = start;
    lane.m_read = start;
    lane.m_consumed.store(start);
    lane.m_published.store(start);
    LibuvPendingSend pending;
    LibuvPendingSend output[73];
    for (uint32_t round = 0; round < 16; ++round)
    {
        for (uint32_t index = 0; index < 1024; ++index)
        {
            pending.m_generation = round * 1024 + index;
            if (!lane.push(pending))
            {
                return false;
            }
        }
        if (lane.push(pending) || lane.pop(output, 0) != 0)
        {
            return false;
        }
        uint32_t count = 0;
        while (count != 1024)
        {
            const size_t taken = lane.pop(output, 73);
            if (taken == 0 || taken > 73 || taken > 1024 - count)
            {
                return false;
            }
            for (size_t index = 0; index < taken; ++index)
            {
                if (output[index].m_generation != round * 1024 + count + index)
                {
                    return false;
                }
            }
            count += static_cast<uint32_t>(taken);
        }
        if (lane.pop(output, 73) != 0)
        {
            return false;
        }
    }
    for (uint32_t size = 0; size <= 16; ++size)
    {
        char outputBytes[18];
        std::memset(outputBytes, 0x5a, sizeof(outputBytes));
        char* input = size == 0 ? nullptr : new char[size];
        for (uint32_t index = 0; index < size; ++index)
        {
            input[index] = static_cast<char>(index + 1);
        }
        LibuvBase::copySmall(outputBytes + 1, input, size);
        bool passed = outputBytes[0] == 0x5a;
        for (uint32_t index = 0; index < 17; ++index)
        {
            passed = outputBytes[index + 1] == (index < size ? static_cast<char>(index + 1) : 0x5a) && passed;
        }
        delete[] input;
        if (!passed)
        {
            return false;
        }
    }
    return true;
}

/** 超过固定分片数的线程同时持有嵌套借用，关闭必须等待每个未归还者
@return 首次关闭、后备计数及再次开放均不遗漏借用
*/
static bool TestShardedGate()
{
    LibuvLifecycleGate gate;
    for (uint32_t round = 0; round < 2; ++round)
    {
        gate.open();
        std::atomic<uint32_t> ready(0);
        std::atomic<bool> release(false);
        std::atomic<bool> failed(false);
        std::atomic<bool> closed(false);
        LibuvTestThread workers[96];
        LibuvTestThread closer;
        const uint64_t deadline = uv_hrtime() + 10000000000ULL;
        uint32_t launched = 0;
        for (; launched < 96; ++launched)
        {
            // 每个任务退出前自己归还所有借用，主线程仅控制已进入后的释放时机。
            if (!workers[launched].start([&]()
            {
                const bool first = gate.acquire();
                const bool second = first && gate.tryAcquire(true) == 0;
                if (second)
                {
                    gate.release();
                }
                else
                {
                    failed.store(true);
                }
                ready.fetch_add(1);
                while (!release.load() && uv_hrtime() < deadline)
                {
                    uv_sleep(1);
                }
                if (first)
                {
                    gate.release();
                }
            }, [&]() { release.store(true); }))
            {
                break;
            }
        }
        while (ready.load() != launched && uv_hrtime() < deadline)
        {
            uv_sleep(1);
        }
        bool passed = launched == 96 && ready.load() == 96 && !failed.load();
        // close可等待，但不能在任何工作线程仍持有借用时返回。
        const bool started = closer.start([&]() { gate.close(); closed.store(true); }, [&]() { release.store(true); });
        bool rejected = false;
        while (started && uv_hrtime() < deadline)
        {
            if (gate.tryAcquire(true) != 0)
            {
                rejected = true;
                break;
            }
            gate.release();
            uv_sleep(0);
        }
        passed = started && rejected && !closed.load() && passed;
        release.store(true);
        for (uint32_t index = 0; index < launched; ++index)
        {
            passed = workers[index].join() && passed;
        }
        if (started)
        {
            passed = closer.join() && passed;
        }
        else
        {
            gate.close();
        }
        if (!passed || failed.load() || !closed.load() || gate.tryAcquire(true) != UV_ECANCELED)
        {
            return false;
        }
    }
    return true;
}

/** 单写和共享两条通道同时有积压时，前两个消费前缀必须分别推进两边
@return 交替消费且各通道序号完整
*/
static bool TestLaneFairness()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1))
    {
        return false;
    }
    LibuvBase* state = tcp.m_libuv;
    LibuvPendingSend pending;
    for (uint32_t index = 0; index < 64; ++index)
    {
        pending.m_generation = index;
        pending.m_close = false;
        if (!state->m_sendLane.push(pending))
        {
            return false;
        }
        pending.m_close = true;
        if (state->m_queue->tryPush(pending, true, false) != LockFreeQueueSuccess)
        {
            return false;
        }
    }
    LibuvPendingSend output[16];
    uint32_t counts[2] = {};
    for (uint32_t round = 0; round < 128 && state->sendsReady(); ++round)
    {
        size_t taken = 0;
        if (!state->popSendPrefix(output, 16, taken) || taken == 0 || taken > 16)
        {
            return false;
        }
        for (size_t index = 0; index < taken; ++index)
        {
            const uint32_t channel = output[index].m_close ? 1 : 0;
            if (output[index].m_generation != counts[channel]++)
            {
                return false;
            }
        }
        if (round == 1 && (counts[0] == 0 || counts[1] == 0))
        {
            return false;
        }
    }
    return counts[0] == 64 && counts[1] == 64 && !state->sendsReady();
}

/** 工作线程持续进入时反复关闭和开放，关闭返回后不允许任何借用访问待替换资源
@return 关闭区间无资源访问，重新开放后工作线程继续推进
*/
static bool TestGateReopenRace()
{
    LibuvLifecycleGate gate;
    gate.open();
    std::atomic<bool> available(true);
    std::atomic<bool> stop(false);
    std::atomic<bool> failed(false);
    std::atomic<uint32_t> ready(0);
    std::atomic<uint32_t> inside(0);
    std::atomic<uint32_t> visits(0);
    LibuvTestThread threads[40];
    const uint64_t deadline = uv_hrtime() + 10000000000ULL;
    uint32_t launched = 0;
    for (; launched < 40; ++launched)
    {
        // gate借用覆盖每一次资源检查；关闭owner在close返回后才把资源标为不可用。
        if (!threads[launched].start([&]()
        {
            ready.fetch_add(1);
            uint32_t loops = 0;
            while (!stop.load() && uv_hrtime() < deadline)
            {
                if (gate.acquire())
                {
                    inside.fetch_add(1);
                    if (!available.load())
                    {
                        failed.store(true);
                    }
                    visits.fetch_add(1);
                    inside.fetch_sub(1);
                    gate.release();
                }
                if ((++loops & 63U) == 0)
                {
                    uv_sleep(0);
                }
            }
        }, [&]() { stop.store(true); }))
        {
            break;
        }
    }
    while (ready.load() != launched && uv_hrtime() < deadline)
    {
        uv_sleep(1);
    }
    bool passed = launched == 40 && ready.load() == 40;
    for (uint32_t round = 0; passed && round < 128; ++round)
    {
        gate.close();
        available.store(false);
        if (inside.load() != 0 || uv_hrtime() >= deadline)
        {
            passed = false;
        }
        uv_sleep(0);
        available.store(true);
        gate.open();
        uv_sleep(0);
    }
    stop.store(true);
    for (uint32_t index = 0; index < launched; ++index)
    {
        passed = threads[index].join() && passed;
    }
    gate.close();
    if (!passed || failed.load() || visits.load() == 0 || inside.load() != 0)
    {
        std::printf("reopen passed=%u failed=%u visits=%u inside=%u launched=%u\n",
            passed ? 1U : 0U, failed.load() ? 1U : 0U, visits.load(), inside.load(), launched);
    }
    return passed && !failed.load() && visits.load() != 0 && inside.load() == 0;
}

/** 解析一个正数用例编号，不调用会抛异常的字符串转换
@param [in,out] input 当前字符，成功后指向首个非数字
@param [out] value 合法编号
@return 编号是否在登记范围内
*/
static bool ParseCaseNumber(const char*& input, uint32_t& value)
{
    value = 0;
    if (*input < '0' || *input > '9')
    {
        return false;
    }
    while (*input >= '0' && *input <= '9')
    {
        value = value * 10 + static_cast<uint32_t>(*input++ - '0');
        if (value > LibuvTestLast)
        {
            return false;
        }
    }
    return value != 0;
}

/** 解析逗号列表和闭区间，重复编号合并，非法输入不产生有效批次
@param [in] input 非空选择文本
@param [out] selected 0 至最大编号的选择表
@return 是否完整解析合法且非空的输入
*/
static bool ParseSelection(const char* input, bool* selected)
{
    std::fill(selected, selected + LibuvTestLast + 1, false);
    if (input == nullptr || *input == 0)
    {
        return false;
    }
    for (;;)
    {
        uint32_t first = 0;
        if (!ParseCaseNumber(input, first))
        {
            return false;
        }
        uint32_t last = first;
        if (*input == '-')
        {
            ++input;
            if (!ParseCaseNumber(input, last) || last < first)
            {
                return false;
            }
        }
        for (uint32_t id = first; id <= last; ++id)
        {
            selected[id] = true;
        }
        if (*input == 0)
        {
            return true;
        }
        if (*input++ != ',' || *input == 0)
        {
            return false;
        }
    }
}

/** 独立验证选择器边界，错误文本不能变成空批次成功
@return 合法列表、去重和非法组合是否符合约定
*/
static bool TestSelector()
{
    bool selected[LibuvTestLast + 1] = {};
    if (!ParseSelection("1,3-5,3,25", selected))
    {
        return false;
    }
    for (uint32_t id = 1; id <= LibuvTestLast; ++id)
    {
        const bool expected = id == 1 || (id >= 3 && id <= 5) || id == 25;
        if (selected[id] != expected)
        {
            return false;
        }
    }
    const char* invalid[] = { "", "0", "-1", "999", "5-2", "1,", ",1", "1,,2", "1-", "1--2", "1x", "99999999999999999999" };
    for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]); ++index)
    {
        if (ParseSelection(invalid[index], selected))
        {
            return false;
        }
    }
    return true;
}

/** 各组参数先保存后初始化，非法设置和运行中设置不能覆盖有效配置
@return 参数范围、组合约束和初始化冻结是否生效
*/
static bool TestPoolConfiguration()
{
    LibuvTcp tcp;
    LibuvQueuePoolConfig queue;
    LibuvConnectionPoolConfig connections;
    LibuvSendPoolConfig send;
    LibuvReadPoolConfig read;
    bool passed = queue.valid() && connections.valid() && send.valid() && read.valid();
    queue.m_initialCapacity = 0;
    passed = tcp.configureSendQueue(queue) == UV_EINVAL && passed;
    queue.m_initialCapacity = 8;
    queue.m_maximumCapacity = 4;
    passed = tcp.configureCloseQueue(queue) == UV_EINVAL && passed;
    queue.m_initialCapacity = 4;
    queue.m_maximumProducers = 1;
    passed = tcp.configureSendQueue(queue) == 0 && !tcp.initServer("127.0.0.1", 0) && passed;
    send.m_producerCacheCapacity = 0;
    passed = tcp.configureSendPool(send) == 0 && passed;
    send.m_laneCapacity = 3;
    passed = tcp.configureSendPool(send) == UV_EINVAL && passed;
    connections.m_lookupCacheCapacity = 3;
    passed = tcp.configureConnectionPool(connections) == UV_EINVAL && passed;
    read.m_bufferBytes = 0;
    passed = tcp.configureReadPool(read) == UV_EINVAL && passed;
    if (!tcp.initServer("127.0.0.1", 0))
    {
        return false;
    }
    queue = LibuvQueuePoolConfig();
    send = LibuvSendPoolConfig();
    read = LibuvReadPoolConfig();
    connections = LibuvConnectionPoolConfig();
    passed = tcp.configureSendQueue(queue) == UV_EBUSY && tcp.configureCloseQueue(queue) == UV_EBUSY &&
        tcp.configureConnectionPool(connections) == UV_EBUSY && tcp.configureSendPool(send) == UV_EBUSY &&
        tcp.configureReadPool(read) == UV_EBUSY && tcp.m_libuv->m_settings.m_sendQueue.m_maximumCapacity == 4 &&
        tcp.m_libuv->m_sendLane.m_capacity == 1024 && passed;
    tcp.stop();
    tcp.loop();
    passed = tcp.configureSendQueue(queue) == 0 && tcp.configureSendPool(send) == 0 &&
        tcp.initServer("127.0.0.1", 0) && passed;
    return passed;
}

/** 小节点池必须在物理上封顶，本库读写缓存按选定数量保留并精确归还预算
@return 实际统计、分配及拒绝结果是否符合公开配置
*/
static bool TestPoolLimits()
{
    LibuvTcp tcp;
    LibuvQueuePoolConfig queue;
    queue.m_initialCapacity = 4;
    queue.m_maximumCapacity = 16;
    queue.m_maximumProducers = 8;
    LibuvSendPoolConfig send;
    send.m_laneCapacity = 0;
    send.m_producerCacheCapacity = 0;
    send.m_maximumPendingBytes = 65536;
    send.m_maximumCachedWrites = 1;
    send.m_maximumActiveWrites = 2;
    LibuvReadPoolConfig read;
    read.m_bufferBytes = 113;
    read.m_maximumCachedBuffers = 1;
    LibuvConnectionPoolConfig connections;
    connections.m_initialCapacity = 4;
    connections.m_maximumCapacity = 8;
    connections.m_directoryCapacity = 2;
    connections.m_protectionRecordCount = 8;
    connections.m_lookupCacheCapacity = 0;
    connections.m_bucketCount = 16;
    if (tcp.configureSendQueue(queue) != 0 || tcp.configureCloseQueue(queue) != 0 ||
        tcp.configureConnectionPool(connections) != 0 || tcp.configureSendPool(send) != 0 ||
        tcp.configureReadPool(read) != 0 || !tcp.initClient("127.0.0.1", 1))
    {
        return false;
    }
    LibuvBase* state = tcp.m_libuv;
    uv_tcp_t* target = state->mainTcp();
    if (!state->registerTcp(target))
    {
        return false;
    }
    uint32_t accepted = 0;
    int32_t result = 0;
    for (; accepted < 32; ++accepted)
    {
        result = tcp.trySend(target, "123456", 6, true, false);
        if (result != 0)
        {
            break;
        }
    }
    const LockFreeQueuePoolStatistics queueStats = state->m_queue->poolStatistics();
    LockFreeHashMapPoolStats mapStats = {};
    bool passed = accepted != 0 && accepted <= 16 && result == UV_ENOBUFS &&
        queueStats.currentCapacity <= 16 && queueStats.peakReservedCapacity <= 16 &&
        state->m_connections.poolStats(mapStats).m_status == LockFreeHashMapOk &&
        mapStats.capacity <= 8 && mapStats.protectionRecords == 8 && mapStats.lookupCacheEntries == 0 &&
        state->m_sendLane.m_items == nullptr && state->m_sendProducers == nullptr;
    LibuvPendingSend pending;
    while (PopSubmitted(state, &pending))
    {
        delete[] pending.m_buffer;
        state->releaseSend(pending.m_charge);
    }
    LibuvPendingSend close;
    uint32_t closes = 0;
    while (closes < 32 && state->m_closeQueue->tryPush(close, true, false) == LockFreeQueueSuccess)
    {
        ++closes;
    }
    const LockFreeQueuePoolStatistics closeStats = state->m_closeQueue->poolStatistics();
    passed = closes != 0 && closes <= 16 && closeStats.peakReservedCapacity <= 16 && passed;
    while (state->m_closeQueue->popReady(&close))
    {
    }
    char* first = state->acquireReadBytes();
    char* second = state->acquireReadBytes();
    passed = first != nullptr && second != nullptr && first != second && passed;
    if (first != nullptr)
    {
        std::memset(first, 1, read.m_bufferBytes);
    }
    state->releaseReadBytes(first);
    state->releaseReadBytes(second);
    passed = state->m_cachedReads == 1 && passed;
    uv_buf_t buffer;
    LibuvBase::AllocateRead(reinterpret_cast<uv_handle_t*>(target), 65536, &buffer);
    passed = buffer.base != nullptr && buffer.len == 113 && passed;
    state->releaseReadBytes(buffer.base);
    LibuvPendingWrite* a = state->acquireWrite();
    LibuvPendingWrite* b = state->acquireWrite();
    if (a == nullptr || b == nullptr)
    {
        delete a;
        delete b;
        return false;
    }
    state->releaseWrite(a);
    state->releaseWrite(b);
    passed = state->m_cachedWrites == 1 && state->m_pendingBytes.load() == 0 && passed;
    std::printf("configured queue=%u accepted=%u physical=%llu closes=%u map=%llu\n",
        queue.m_maximumCapacity, accepted, static_cast<unsigned long long>(queueStats.currentCapacity),
        closes, static_cast<unsigned long long>(mapStats.capacity));
    return passed;
}

/** 关闭或缩小高速缓存仍能在四发送线程下保持十二种入口的顺序
@return 真实接收是否完整且默认协议不变
*/
static bool TestConfiguredTraffic()
{
    for (uint32_t mode = 0; mode < 3; ++mode)
    {
        LibuvPoolSettings pools;
        pools.m_sendQueue.m_initialCapacity = 8;
        pools.m_sendQueue.m_maximumCapacity = 32;
        pools.m_sendQueue.m_maximumProducers = 8;
        pools.m_send.m_laneCapacity = mode == 0 ? 0 : (mode == 1 ? 1 : 8);
        pools.m_send.m_producerCacheCapacity = mode == 0 ? 0 : mode + 1;
        pools.m_send.m_maximumActiveWrites = 1;
        pools.m_send.m_maximumCachedWrites = mode;
        pools.m_send.m_maximumPendingBytes = 4096;
        pools.m_read.m_bufferBytes = 127;
        pools.m_read.m_maximumCachedBuffers = mode;
        if (!TestTraffic(4, 1000, 64, false, false, true, &pools))
        {
            return false;
        }
    }
    return true;
}

/** 原始字节路径的配置必须在启动前生效，背压按配置精确计算且能恢复
@return 两次写的自定义窗口、读缓冲与缓存禁用是否符合约定
*/
static bool TestHostPoolConfiguration()
{
    LibuvTestHost server;
    LibuvTestHost client;
    LibuvChannelWritePoolConfig write;
    write.m_maximumWriteBytes = 8;
    write.m_maximumPendingBytesPerPeer = 16;
    write.m_maximumWritesPerPeer = 2;
    write.m_maximumCachedWrites = 0;
    write.m_maximumCachedBytes = 0;
    if (!server.initialize() || !client.initialize() ||
        LibuvTcp::hostConfigureReadBuffer(server.m_instance, 3) != 0 ||
        LibuvTcp::hostConfigureReadBuffer(server.m_instance, 0) != UV_EINVAL ||
        LibuvTcp::hostConfigureWritePool(client.m_instance, write) != 0)
    {
        return false;
    }
    uint64_t peer = 0;
    if (LibuvTcp::hostListen(server.m_instance, "127.0.0.1", 0, 1) != 0 ||
        LibuvTcp::hostConnect(client.m_instance, "127.0.0.1", LibuvTcp::hostLocalPort(server.m_instance), &peer) != 0 ||
        !PollHosts(server, client, [&]() { return server.m_connected == 1 && client.m_connected == 1; }))
    {
        return false;
    }
    const uint8_t bytes[9] = { 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A };
    bool passed = LibuvTcp::hostConfigureWritePool(client.m_instance, write) == UV_EBUSY &&
        LibuvTcp::hostConfigureReadBuffer(server.m_instance, 7) == UV_EBUSY &&
        LibuvTcp::hostWrite(client.m_instance, peer, bytes, 9) < 0 &&
        LibuvTcp::hostWrite(client.m_instance, peer, bytes, 8) == 0 &&
        LibuvTcp::hostWrite(client.m_instance, peer, bytes, 8) == 0 &&
        LibuvTcp::hostWrite(client.m_instance, peer, bytes, 1) == 1;
    passed = PollHosts(server, client, [&]() { return server.m_received == 16 && client.m_written == 2; }) && passed;
    passed = LibuvTcp::hostWrite(client.m_instance, peer, bytes, 8) == 0 && passed;
    passed = PollHosts(server, client, [&]() { return server.m_received == 24 && client.m_written == 3; }) && passed;
    passed = client.finish() && server.finish() && !server.m_failed && !client.m_failed && passed;
    LibuvChannelState state;
    state.m_writeConfig = write;
    LibuvChannelWrite* request = state.prepareWrite(nullptr, bytes, 8);
    if (request == nullptr)
    {
        return false;
    }
    state.releaseWrite(request);
    return passed && !state.m_loopInitialized && state.m_cachedWrites == 0 && state.m_cachedWriteBytes == 0;
}

/** 指定池分配失败后必须完整回滚且允许再次初始化
@return 失败没有发布实例，恢复分配后重试成功
*/
static bool TestPoolAllocationFailure()
{
    LibuvTcp tcp;
    LibuvSendPoolConfig send;
    send.m_laneCapacity = 16;
    if (tcp.configureSendPool(send) != 0)
    {
        return false;
    }
    LibuvTcpTest::m_failAllocationSize.store(16 * sizeof(LibuvPendingSend));
    LibuvTcpTest::m_failAllocationCount.store(1);
    const bool failed = !tcp.initServer("127.0.0.1", 0);
    const bool injected = LibuvTcpTest::m_failAllocationCount.exchange(0) == 0;
    LibuvTcpTest::m_failAllocationSize.store(0);
    return injected && failed && tcp.m_libuv == nullptr && tcp.initServer("127.0.0.1", 0);
}

/** 超过配置预算的请求不得无限等待，try失败保持所有权
@return 普通、try及批量入口是否及时拒绝不可满足的请求
*/
static bool TestConfiguredByteBudget()
{
    LibuvTcp tcp;
    LibuvSendPoolConfig send;
    send.m_maximumPendingBytes = 1;
    if (tcp.configureSendPool(send) != 0 || !tcp.initClient("127.0.0.1", 1) ||
        !tcp.m_libuv->registerTcp(tcp.m_libuv->mainTcp()))
    {
        return false;
    }
    uv_tcp_t* target = tcp.m_libuv->mainTcp();
    LibuvSendBuffer item = { "123456", 6 };
    return tcp.trySend(target, item.m_data, 6, true, true) == UV_EINVAL &&
        tcp.tryPostBatch(target, &item, 1, true, true) == UV_EINVAL &&
        !tcp.send(target, item.m_data, 6) && !tcp.postBatch(target, &item, 1) &&
        tcp.m_libuv->m_pendingBytes.load() == 0;
}

/** 在同一执行者上建立真实回环连接，测试代码可精确选择投递与poll的交错
@param [in,out] receiver 尚未初始化的接收实例
@param [in,out] client 尚未初始化的发送实例
@return 是否完成连接；失败时实例析构仍负责资源收敛
*/
static bool ConnectDirectTest(LibuvTinyReceiver& receiver, LibuvTestClient& client)
{
    if (!receiver.initServer("127.0.0.1", 0))
    {
        return false;
    }
    sockaddr_in address = {};
    int size = sizeof(address);
    if (uv_tcp_getsockname(receiver.m_libuv->mainTcp(), reinterpret_cast<sockaddr*>(&address), &size) != 0 ||
        !client.initClient("127.0.0.1", ntohs(address.sin_port)))
    {
        return false;
    }
    const uint64_t deadline = uv_hrtime() + 3000000000ULL;
    while (client.m_target.load() == nullptr && uv_hrtime() < deadline)
    {
        uv_run(receiver.loopPtr(), UV_RUN_NOWAIT);
        uv_run(client.loopPtr(), UV_RUN_NOWAIT);
    }
    return client.m_target.load() != nullptr && !client.m_failed.load();
}

/** 同步前缀通过真实socket交付，不触发原有合批数组分配
@return 接收完整、预算归零且精确大小的分配故障未被消耗
*/
static bool TestDirectPrefixAllocation()
{
    LibuvTinyReceiver receiver(100, false, 1);
    LibuvTestClient client;
    if (!ConnectDirectTest(receiver, client))
    {
        return false;
    }
    for (uint32_t index = 0; index < 100; ++index)
    {
        if (client.trySend(client.m_target.load(), "123456", 6, true, false) != 0)
        {
            return false;
        }
    }
    LibuvTcpTest::m_failAllocationSize.store(sizeof(uv_tcp_t*) + 1000);
    LibuvTcpTest::m_failAllocationCount.store(1);
    LibuvBase::SendPending(client.m_libuv->m_asyncHandle);
    bool passed = LibuvTcpTest::m_failAllocationCount.load() == 1 &&
        client.m_libuv->m_pendingBytes.load() == 0 && client.m_libuv->m_activeWrites == 0;
    LibuvTcpTest::m_failAllocationSize.store(0);
    LibuvTcpTest::m_failAllocationCount.store(0);
    const uint64_t deadline = uv_hrtime() + 3000000000ULL;
    while (receiver.m_received.load() != 100 && !receiver.m_failed.load() && uv_hrtime() < deadline)
    {
        uv_run(receiver.loopPtr(), UV_RUN_NOWAIT);
        uv_run(client.loopPtr(), UV_RUN_NOWAIT);
    }
    passed = receiver.m_received.load() == 100 && !receiver.m_failed.load() && passed;
    client.stop();
    client.loop();
    receiver.stop();
    receiver.loop();
    return passed;
}

/** 真实发送长度头或正文的一部分，再让生产路径接续尾部；不伪造已发送字节
@param [in] failWrite true注入异步写请求分配失败，核对尾部收敛；false核对完整数据
@return 四个分割位置不重复发送，失败仍释放所有数组和预算
*/
static bool TestDirectPrefixPartial(bool failWrite)
{
    const uint32_t split[] = {1, 3, 4, 9};
    for (size_t mode = 0; mode < sizeof(split) / sizeof(split[0]); ++mode)
    {
        LibuvTinyReceiver receiver(1, false, 1);
        LibuvTestClient client;
        if (!ConnectDirectTest(receiver, client))
        {
            return false;
        }
        LibuvPendingSend pending;
        pending.m_target = client.m_target.load();
        pending.m_wireSize = 10;
        pending.m_charge = static_cast<uint32_t>(sizeof(uv_tcp_t*) + 10 + sizeof(LibuvPendingWrite));
        if (!client.m_libuv->connectionGeneration(pending.m_target, pending.m_generation) ||
            client.m_libuv->tryReserveSend(pending.m_charge, true) != 0)
        {
            return false;
        }
        pending.m_buffer = client.memoryForSend(static_cast<int32_t>(sizeof(uv_tcp_t*) + 10));
        const uint32_t length = 6;
        std::memcpy(pending.m_buffer, &pending.m_target, sizeof(uv_tcp_t*));
        std::memcpy(pending.m_buffer + sizeof(uv_tcp_t*), &length, 4);
        std::memcpy(pending.m_buffer + sizeof(uv_tcp_t*) + 4, "123456", 6);
        uv_buf_t prefix = uv_buf_init(pending.m_buffer + sizeof(uv_tcp_t*), split[mode]);
        if (uv_try_write(reinterpret_cast<uv_stream_t*>(pending.m_target), &prefix, 1) != static_cast<int32_t>(split[mode]))
        {
            client.m_libuv->releaseSend(pending.m_charge);
            client.releaseSendMemory(pending.m_buffer);
            return false;
        }
        LibuvTcpTest::m_watchedArray.store(pending.m_buffer);
        LibuvTcpTest::m_arrayDeletes.store(0);
        LibuvTcpTest::m_failAllocationSize.store(failWrite ? sizeof(LibuvPendingWrite) : 0);
        LibuvTcpTest::m_failAllocationCount.store(failWrite ? 1 : 0);
        client.m_libuv->processPending(pending, split[mode]);
        bool passed = !failWrite || LibuvTcpTest::m_failAllocationCount.load() == 0;
        LibuvTcpTest::m_failAllocationSize.store(0);
        LibuvTcpTest::m_failAllocationCount.store(0);
        const uint64_t deadline = uv_hrtime() + 3000000000ULL;
        while (!failWrite && receiver.m_received.load() != 1 && !receiver.m_failed.load() && uv_hrtime() < deadline)
        {
            uv_run(client.loopPtr(), UV_RUN_NOWAIT);
            uv_run(receiver.loopPtr(), UV_RUN_NOWAIT);
        }
        client.stop();
        client.loop();
        receiver.stop();
        receiver.loop();
        passed = passed && LibuvTcpTest::m_arrayDeletes.load() == 1 &&
            (failWrite ? receiver.m_received.load() == 0 : receiver.m_received.load() == 1 && !receiver.m_failed.load());
        LibuvTcpTest::m_watchedArray.store(nullptr);
        if (!passed)
        {
            return false;
        }
    }
    return true;
}


/** 附加独占通道与共享队列均有积压时，轮转不得遗漏或跨线程借用未发布数据
@return 总槽数有界、全部通道推进、局部顺序保持、初始化分配失败可安全回收
*/
static bool TestProducerLanes()
{
    LibuvTcp tcp;
    if (!tcp.initClient("127.0.0.1", 1))
    {
        return false;
    }
    LibuvBase* state = tcp.m_libuv;
    const uint32_t laneSize = 512;
    uint32_t capacity = 0;
    LibuvPendingSend pending;
    for (uint32_t slot = 0; slot < 16; ++slot)
    {
        LibuvSendLane& lane = state->m_sendProducers[slot].m_lane;
        capacity += lane.m_capacity;
        if (lane.m_capacity != laneSize)
        {
            return false;
        }
        for (uint32_t index = 0; index < laneSize; ++index)
        {
            pending.m_generation = uint64_t(slot) * laneSize + index;
            if (!lane.push(pending))
            {
                return false;
            }
        }
        if (lane.push(pending))
        {
            return false;
        }
    }
    if (capacity > state->m_settings.m_sendQueue.m_maximumCapacity)
    {
        return false;
    }
    pending.m_generation = 16 * laneSize;
    if (state->m_queue->tryPush(pending, true, false) != LockFreeQueueSuccess)
    {
        return false;
    }
    uint32_t counts[17] = {};
    for (uint32_t round = 0; round < 16 * laneSize + 1; ++round)
    {
        if (!PopSubmitted(state, &pending))
        {
            return false;
        }
        const uint32_t slot = static_cast<uint32_t>(pending.m_generation / laneSize);
        const uint32_t index = static_cast<uint32_t>(pending.m_generation % laneSize);
        if (slot > 16 || index != counts[slot]++)
        {
            return false;
        }
        if (round == 16)
        {
            for (uint32_t lane = 0; lane < 17; ++lane)
            {
                if (counts[lane] == 0)
                {
                    return false;
                }
            }
        }
    }
    if (state->sendsReady() || PopSubmitted(state, &pending))
    {
        return false;
    }
    // 真实拥有数组与预算的附加通道在停止后仍按1024项轮次完整回收。
    const uint32_t charge = static_cast<uint32_t>(sizeof(uv_tcp_t*) + 5 + sizeof(LibuvPendingWrite));
    for (uint32_t index = 0; index < 1028; ++index)
    {
        pending.m_buffer = new (std::nothrow) char[sizeof(uv_tcp_t*) + 5];
        if (pending.m_buffer == nullptr)
        {
            return false;
        }
        if (state->tryReserveSend(charge, true) != 0)
        {
            delete[] pending.m_buffer;
            return false;
        }
        pending.m_charge = charge;
        pending.m_wireSize = 5;
        LibuvSendLane& lane = index < 1024 ? state->m_sendProducers[index / 64].m_lane : state->m_sendLane;
        if (!lane.push(pending))
        {
            delete[] pending.m_buffer;
            state->releaseSend(charge);
            return false;
        }
    }
    tcp.stop();
    LibuvBase::SendPending(state->m_asyncHandle);
    if (state->m_pendingBytes.load() != charge * 4)
    {
        return false;
    }
    LibuvBase::SendPending(state->m_asyncHandle);
    if (state->m_pendingBytes.load() != 0 || state->sendsReady())
    {
        return false;
    }
    // 正好命中附加512项描述符数组；失败不能发布半初始化状态。
    LibuvTcp failure;
    LibuvTcpTest::m_failAllocationSize.store(sizeof(LibuvPendingSend) * laneSize);
    LibuvTcpTest::m_failAllocationCount.store(1);
    const bool initialized = failure.initClient("127.0.0.1", 1);
    const bool injected = LibuvTcpTest::m_failAllocationCount.load() == 0;
    LibuvTcpTest::m_failAllocationSize.store(0);
    LibuvTcpTest::m_failAllocationCount.store(0);
    return !initialized && injected && failure.m_libuv == nullptr && failure.initClient("127.0.0.1", 1);
}

int LibuvTcpTest::run(int argc, char** argv)
{
    static const std::map<int32_t, LibuvTestCase> s_cases =
    {
        { LibuvTestPerformanceParser, { "性能样本完整性与异常数值拒绝", false, &LibuvTcpPerformance::checkParser } },
        { LibuvTestPerformanceRunner, { "批次子进程异常超时与输出上限", false, &LibuvTcpPerformance::checkRunner } },
#ifndef LIBUV_TEST_BASELINE
        { LibuvTestPoolConfiguration, { "池配置校验、组合约束和初始化冻结", false, &TestPoolConfiguration } },
        { LibuvTestPoolLimits, { "实际节点上限和读写缓存封顶", false, &TestPoolLimits } },
        { LibuvTestConfiguredTraffic, { "小容量四线程十二入口保序", true, &TestConfiguredTraffic } },
        { LibuvTestHostPoolConfiguration, { "host自定义写窗口、读块和缓存禁用", false, &TestHostPoolConfiguration } },
        { LibuvTestPoolAllocationFailure, { "池分配失败回滚后重试初始化", false, &TestPoolAllocationFailure } },
        { LibuvTestConfiguredByteBudget, { "不足一条消息的预算不永久等待", false, &TestConfiguredByteBudget } },
        { LibuvTestProducerLanes, { "附加通道有界、轮转与故障回收", false, &TestProducerLanes } },
        { LibuvTestDirectPrefixAllocation, { "同步前缀不申请合批数组", false, &TestDirectPrefixAllocation } },
        { LibuvTestDirectPrefixPartial, { "四种真实部分写入尾部接续", false, []() { return TestDirectPrefixPartial(false); } } },
        { LibuvTestDirectPrefixFailure, { "部分写入后分配失败恰好回收", false, []() { return TestDirectPrefixPartial(true); } } },
        { LibuvTestGateReopenRace, { "40线程持续进入与128次关闭重开", true, &TestGateReopenRace } },
        { LibuvTestLaneBoundaries, { "单写通道容量回绕和精确拷贝", false, &TestLaneBoundaries } },
        { LibuvTestShardedGate, { "96线程借用分片碰撞嵌套与关闭", true, &TestShardedGate } },
        { LibuvTestLaneFairness, { "单写及并发通道交替消费", false, &TestLaneFairness } },
        { LibuvTestRejectedSend, { "满队列发送数组完整释放", false, &TestRejectedSend } },
        { LibuvTestRejectedClose, { "满取消队列收敛停止", false, &TestRejectedClose } },
#else
        // 新 Queue 的普通 push 等待容量；旧封装不能安全运行满队列用例，不能把未执行算通过。
        { LibuvTestRejectedSend, { "满队列发送数组完整释放", false, nullptr } },
        { LibuvTestRejectedClose, { "满取消队列收敛停止", false, nullptr } },
#endif
        { LibuvTestSingleProducer, { "单生产者双向字节完整", false, []() { return TestTraffic(1, 1000, 64); } } },
        { LibuvTestConcurrentOrder, { "四生产者各自保序", false, []() { return TestTraffic(4, 1000, 1024); } } },
        { LibuvTestReadBuffers, { "在途读缓冲隔离", false, &TestReadBuffers } },
#ifndef LIBUV_TEST_BASELINE
        { LibuvTestSendBudget, { "发送预算边界", false, &TestSendBudget } },
        { LibuvTestStoppedPrefix, { "停止批次完整回收", false, &TestStoppedPrefix } },
        { LibuvTestStaleGeneration, { "旧连接代次隔离", false, &TestStaleGeneration } },
        { LibuvTestWriteAllocation, { "写分配失败预算归还", false, &TestWriteAllocationFailure } },
        { LibuvTestCloseBeforeData, { "关闭取消未处理数据", false, &TestCloseBeforeData } },
        { LibuvTestContention, { "八线程普通 CAS 竞争不拒绝", true, &TestContentionDoesNotReject } },
        { LibuvTestActualCapacity, { "真实发送容量保护", false, &TestActualSendCapacity } },
        { LibuvTestWindowResume, { "满写窗口恢复完整交付", true, []() { return TestWriteWindow(false); } } },
        { LibuvTestWindowCancel, { "满写窗口及时取消", true, []() { return TestWriteWindow(true); } } },
#else
        { LibuvTestSendBudget, { "发送预算边界", false, nullptr } },
        { LibuvTestStoppedPrefix, { "停止批次完整回收", false, nullptr } },
        { LibuvTestStaleGeneration, { "旧连接代次隔离", false, nullptr } },
        { LibuvTestWriteAllocation, { "写分配失败预算归还", false, nullptr } },
        { LibuvTestCloseBeforeData, { "关闭取消未处理数据", false, nullptr } },
        { LibuvTestContention, { "八线程普通 CAS 竞争不拒绝", true, nullptr } },
        { LibuvTestActualCapacity, { "真实发送容量保护", false, nullptr } },
        { LibuvTestWindowResume, { "满写窗口恢复完整交付", true, nullptr } },
        { LibuvTestWindowCancel, { "满写窗口及时取消", true, nullptr } },
#endif
        { LibuvTestHostContracts, { "host 生命周期和重入", false, &TestHostContracts } },
        { LibuvTestHostBackpressure, { "host 背压和暂停恢复", false, &TestHostBackpressure } },
        { LibuvTestHostExceptions, { "四种回调异常收敛", false, &TestHostExceptions } },
        { LibuvTestRepeatedLifecycle, { "256 轮生命周期句柄回落", true, &TestRepeatedLifecycle } },
        { LibuvTestBorrowClose, { "借用与关闭固定交错", false, &TestBorrowClose } },
        { LibuvTestAcceptAllocation, { "64 次接纳分配失败后恢复", false, &TestAcceptAllocationRecovery } },
        { LibuvTestStopRace, { "四生产者发送中停止", true, []() { return TestTraffic(4, 5000, 64, true); } } },
        { LibuvTestLargeFrames, { "八生产者 128MiB 大帧校验", true, []() { return TestTraffic(8, 256, 65536); } } },
        { LibuvTestInitialization, { "无效初始化及串行重建", false, &TestInitializationBoundaries } },
        { LibuvTestUnpublishedAccept, { "128 次未发布接纳不报终态", false, &TestUnpublishedAccept } },
        { LibuvTestSelector, { "测试选择器边界", false, &TestSelector } },
#ifndef LIBUV_TEST_BASELINE
        { LibuvTestSpaceWaitResume, { "容量等待后128MiB完整交付", true, []() { return TestSpaceWait(false); } } },
        { LibuvTestSpaceWaitStop, { "容量等待期间停止收敛", true, []() { return TestSpaceWait(true); } } },
        { LibuvTestTryOptions, { "try双选项和所有权边界", false, &TestTryOptions } },
        { LibuvTestCloseCapacity, { "满取消队列等待而不误停实例", true, &TestCloseCapacity } },
#else
        { LibuvTestSpaceWaitResume, { "容量等待后128MiB完整交付", true, nullptr } },
        { LibuvTestSpaceWaitStop, { "容量等待期间停止收敛", true, nullptr } },
        { LibuvTestTryOptions, { "try双选项和所有权边界", false, nullptr } },
        { LibuvTestCloseCapacity, { "满取消队列等待而不误停实例", true, nullptr } },
#endif
#ifndef LIBUV_TEST_BEFORE_BATCH
        { LibuvTestBatchContract, { "批量参数边界与独立复制", false, &TestBatchContract } },
        { LibuvTestBatchFailures, { "批量容量四选项与分配失败", false, &TestBatchFailures } },
        { LibuvTestBatchOrder, { "六字节批量并发顺序和尾批", true, &TestBatchOrder } },
        { LibuvTestBatchMixed, { "单帧批量混合逐线程保序", true, []() { return LibuvTcpBenchmark::tiny(4, 2, 100003, 64) == 0; } } },
        { LibuvTestMergeAllocation, { "自动合批分配失败回收", false, &TestMergeAllocation } },
        { LibuvTestBatchStopped, { "停止批量操作完整回收", false, &TestBatchStopped } },
        { LibuvTestBatchBackpressure, { "窄发送缓冲与延迟接收整批恢复", true, []() { return LibuvTcpBenchmark::tiny(4, 1, 100003, 4096, true) == 0; } } },
        { LibuvTestWakeEdges, { "逐帧空队列唤醒完整交付", true, []() { return LibuvTcpBenchmark::run(1, 16, 2000, 1) == 0; } } },
#endif
#ifndef LIBUV_TEST_BEFORE_COPY
        { LibuvTestCopyContract, { "逐条正文参数和内联边界", false, &TestCopyContract } },
        { LibuvTestCopyFailures, { "内联分配与容量失败四组合", false, &TestCopyFailures } },
        { LibuvTestCopyOrder, { "逐条正文并发保序", true, []() { return LibuvTcpBenchmark::tiny(4, 1, 100003, 1, false, true) == 0; } } },
        { LibuvTestCopyMixed, { "数组正文和批量混合保序", true, []() { return LibuvTcpBenchmark::tiny(4, 2, 100003, 64, false, true) == 0; } } },
        { LibuvTestCopyBackpressure, { "逐条正文背压恢复", true, []() { return LibuvTcpBenchmark::tiny(4, 1, 100003, 1, true, true) == 0; } } },
        { LibuvTestConnectionCache, { "连接缓存碰撞和撤销换代", false, &TestConnectionCache } },
        { LibuvTestInlineWriteFailure, { "内联写请求分配失败回收", false, &TestInlineWriteFailure } },
        { LibuvTestCopySizes, { "逐条正文六种大小及并发停止", true, &TestCopySizes } },
        { LibuvTestPostContract, { "post正文批量边界与副本", false, &TestPostContract } },
        { LibuvTestPostFailures, { "post所有权和三入口重试四组合", false, &TestPostFailures } },
        { LibuvTestPostMixed, { "十二种send/post入口并发保序", true, []() { return TestTraffic(4, 10003, 64, false, false, true); } } },
        { LibuvTestPostSizes, { "混合post内联大帧与并发停止", true, &TestPostSizes } },
        { LibuvTestPostWait, { "三种post满预算恢复128MiB", true, []() { return TestPostWait(false); } } },
        { LibuvTestPostCancel, { "三种post满预算停止解阻", true, []() { return TestPostWait(true); } } },
#else
        { LibuvTestCopyContract, { "逐条正文参数和内联边界", false, nullptr } },
        { LibuvTestCopyFailures, { "内联分配与容量失败四组合", false, nullptr } },
        { LibuvTestCopyOrder, { "逐条正文并发保序", true, nullptr } },
        { LibuvTestCopyMixed, { "数组正文和批量混合保序", true, nullptr } },
        { LibuvTestCopyBackpressure, { "逐条正文背压恢复", true, nullptr } },
        { LibuvTestConnectionCache, { "连接缓存碰撞和撤销换代", false, nullptr } },
        { LibuvTestInlineWriteFailure, { "内联写请求分配失败回收", false, nullptr } },
        { LibuvTestCopySizes, { "逐条正文六种大小及并发停止", true, nullptr } },
        { LibuvTestPostContract, { "post正文批量边界与副本", false, nullptr } },
        { LibuvTestPostFailures, { "post所有权和三入口重试四组合", false, nullptr } },
        { LibuvTestPostMixed, { "十二种send/post入口并发保序", true, nullptr } },
        { LibuvTestPostSizes, { "混合post内联大帧与并发停止", true, nullptr } },
        { LibuvTestPostWait, { "三种post满预算恢复128MiB", true, nullptr } },
        { LibuvTestPostCancel, { "三种post满预算停止解阻", true, nullptr } },
#endif
#ifndef LIBUV_TEST_BEFORE_10M
        { LibuvTestProducerFallback, { "生产者缓存重入和冲突回退", false, &TestProducerFallback } },
        { LibuvTestProducerConcurrent, { "四十线程缓存与普通入口保序", true, &TestProducerConcurrent } },
        { LibuvTestProducerLifecycle, { "缓存登记128次停止重建", true, &TestProducerLifecycle } },
        { LibuvTestCapacityConcurrentResume, { "16线程满队列恢复保序", true, []() { return TestCapacityConcurrent(false); } } },
        { LibuvTestCapacityConcurrentStop, { "16线程满队列停止解阻", true, []() { return TestCapacityConcurrent(true); } } },
#else
        { LibuvTestProducerFallback, { "生产者缓存重入和冲突回退", false, nullptr } },
        { LibuvTestProducerConcurrent, { "四十线程缓存与普通入口保序", true, nullptr } },
        { LibuvTestProducerLifecycle, { "缓存登记128次停止重建", true, nullptr } },
        { LibuvTestCapacityConcurrentResume, { "16线程满队列恢复保序", true, nullptr } },
        { LibuvTestCapacityConcurrentStop, { "16线程满队列停止解阻", true, nullptr } },
#endif
    };
    bool selected[LibuvTestLast + 1] = {};
    if (s_cases.size() != LibuvTestLast)
    {
        return 2;
    }
    if (argc == 3 && std::strcmp(argv[1], "--case") == 0)
    {
        if (!ParseSelection(argv[2], selected))
        {
            std::fputs("invalid test selection\n", stderr);
            return 2;
        }
    }
    else
    {
        const char* mode = argc == 1 ? "--all" : argv[1];
        if (argc > 2 || (std::strcmp(mode, "--all") != 0 && std::strcmp(mode, "--instant") != 0 &&
            std::strcmp(mode, "--stress") != 0 && std::strcmp(mode, "--list") != 0))
        {
            std::fputs("usage: --all | --instant | --stress | --list | --case 1,3-5\n", stderr);
            return 2;
        }
        for (auto entry = s_cases.begin(); entry != s_cases.end(); ++entry)
        {
            if (std::strcmp(mode, "--list") == 0)
            {
                std::printf("%d %s %s\n", entry->first, entry->second.m_stress ? "stress" : "instant", entry->second.m_name);
            }
            selected[entry->first] = std::strcmp(mode, "--all") == 0 ||
                (entry->second.m_stress ? std::strcmp(mode, "--stress") == 0 : std::strcmp(mode, "--instant") == 0);
        }
        if (std::strcmp(mode, "--list") == 0)
        {
            return 0;
        }
    }
    uint32_t executed = 0;
    uint32_t failed = 0;
    uint32_t skipped = 0;
    for (auto entry = s_cases.begin(); entry != s_cases.end(); ++entry)
    {
        if (!selected[entry->first])
        {
            continue;
        }
        if (entry->second.m_run == nullptr)
        {
            ++skipped;
            std::printf("case=%d SKIP baseline-layout %s\n", entry->first, entry->second.m_name);
            continue;
        }
        const uint64_t begin = uv_hrtime();
        bool passed = false;
        try
        {
            passed = entry->second.m_run();
        }
        catch (...)
        {
            // 测试注入或意外异常不能当作通过；资源由该用例的 RAII 收尾。
        }
        m_failAllocationCount.store(0);
        m_failAllocationSize.store(0);
        m_watchedArray.store(nullptr);
        ++executed;
        failed += passed ? 0 : 1;
        std::printf("case=%d %s ms=%.3f %s\n", entry->first, passed ? "PASS" : "FAIL",
            double(uv_hrtime() - begin) / 1000000.0, entry->second.m_name);
        std::fflush(stdout);
    }
    std::printf("executed=%u passed=%u failed=%u skipped=%u\n", executed, executed - failed, failed, skipped);
    return executed != 0 && failed == 0 ? 0 : 1;
}

/** 独立测试进程入口；异常属于失败，不能作为通过或中断其他测试宿主
@param [in] argc 参数个数
@param [in] argv 测试选择或性能运行参数
@return 测试退出状态
*/
int main(int argc, char** argv)
{
    try
    {
        if (argc == 3 && std::strcmp(argv[1], "--runner-probe") == 0)
        {
            return LibuvTcpPerformance::runnerProbe(argv[2]);
        }
#ifdef LIBUV_TEST_BENCHMARK
        if (argc >= 2 && std::strcmp(argv[1], "--suite") == 0)
        {
            return LibuvTcpPerformance::run(argc, argv);
        }
        if (argc == 2 && std::strcmp(argv[1], "--profile") == 0)
        {
            return LibuvTcpBenchmark::profile();
        }
        if (argc != 6 || (std::strcmp(argv[1], "--benchmark") != 0 && std::strcmp(argv[1], "--probe") != 0 &&
            std::strcmp(argv[1], "--rawbenchmark") != 0 && std::strcmp(argv[1], "--tinybenchmark") != 0 &&
            std::strcmp(argv[1], "--tinycopybenchmark") != 0 && std::strcmp(argv[1], "--tinypostbenchmark") != 0 &&
            std::strcmp(argv[1], "--tinycopypostbenchmark") != 0))
        {
            std::fputs("usage: --benchmark producers payload count window\n", stderr);
            return 2;
        }
        uint32_t values[4] = {};
        for (size_t index = 0; index < 4; ++index)
        {
            const char* input = argv[index + 2];
            char* end = nullptr;
            errno = 0;
            const unsigned long parsed = std::strtoul(input, &end, 10);
            if (*input < '0' || *input > '9' || end == input || *end != 0 || errno != 0 || parsed > 0xffffffffUL)
            {
                return 2;
            }
            values[index] = static_cast<uint32_t>(parsed);
        }
        if (std::strcmp(argv[1], "--tinybenchmark") == 0)
        {
            return LibuvTcpBenchmark::tiny(values[0], values[1], values[2], values[3]);
        }
        if (std::strcmp(argv[1], "--tinycopybenchmark") == 0)
        {
#ifdef LIBUV_TEST_BEFORE_COPY
            return 2;
#else
            if (values[3] != 1)
            {
                return 2;
            }
            return LibuvTcpBenchmark::tiny(values[0], values[1], values[2], values[3], false, true);
#endif
        }
        if (std::strcmp(argv[1], "--tinypostbenchmark") == 0 || std::strcmp(argv[1], "--tinycopypostbenchmark") == 0)
        {
            if (values[3] != 1)
            {
                return 2;
            }
            return LibuvTcpBenchmark::tiny(values[0], values[1], values[2], 1, false,
                std::strcmp(argv[1], "--tinycopypostbenchmark") == 0, true);
        }
        if (std::strcmp(argv[1], "--rawbenchmark") == 0)
        {
            if (values[0] != 1 || values[3] != 32)
            {
                return 2;
            }
            double milliseconds = 0;
            const int result = LibuvTcpPublicTest::run(values[1], values[2], milliseconds);
            std::printf("RAW,%u,%u,%.6f,%.3f\n", values[1], values[2], milliseconds,
                milliseconds > 0 ? values[2] * 1000.0 / milliseconds : 0);
            return result;
        }
        return LibuvTcpBenchmark::run(values[0], values[1], values[2], values[3], std::strcmp(argv[1], "--probe") == 0);
#else
        (void)argc;
        (void)argv;
        return LibuvTcpTest::run(argc, argv);
#endif
    }
    catch (...)
    {
        std::fputs("test process caught unexpected exception\n", stderr);
        return 2;
    }
}