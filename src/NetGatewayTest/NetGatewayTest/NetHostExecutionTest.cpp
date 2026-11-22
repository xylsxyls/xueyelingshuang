#include "NetHostExecutionTest.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#ifndef NET_HOST_EXECUTION_STANDALONE
#include "NetInterface/NetInterfaceAPI.h"
#endif
#include <chrono>
#include <cstdio>
#include <thread>
#include <stdexcept>

NetHostExecutionTest::NetHostExecutionTest() :
m_peer(0), m_received(0), m_closed(0), m_failed(false), m_loop(nullptr)
{

}

void NetHostExecutionTest::connected(void* context, uint64_t peer)
{
    NetHostExecutionTest& test = *static_cast<NetHostExecutionTest*>(context);
    test.m_peer = peer;
    if (LibuvTcp::hostLoopWait(test.m_loop, LibuvTcp::hostLoopBegin(test.m_loop), 1) >= 0 ||
        LibuvTcp::hostLoopDestroy(test.m_loop) >= 0)
    {
        test.m_failed = true;
    }
}

void NetHostExecutionTest::data(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    (void)peer;
    NetHostExecutionTest& test = *static_cast<NetHostExecutionTest*>(context);
    for (uint32_t index = 0; index < size; ++index)
    {
        if (bytes[index] != static_cast<uint8_t>('1' + test.m_received % 6))
        {
            test.m_failed = true;
        }
        ++test.m_received;
    }
}

void NetHostExecutionTest::writable(void* context, uint64_t peer)
{
    (void)context;
    (void)peer;
}

void NetHostExecutionTest::closed(void* context, uint64_t peer, int32_t reason)
{
    (void)peer;
    (void)reason;
    ++static_cast<NetHostExecutionTest*>(context)->m_closed;
}

int NetHostExecutionTest::run()
{
    void* loop = nullptr;
    if (LibuvTcp::hostLoopCreate(&loop) != 0) { return 1; }
    const auto started = std::chrono::steady_clock::now();
    if (LibuvTcp::hostLoopWait(loop, LibuvTcp::hostLoopBegin(loop), 40) != 0) { return 2; }
    const auto idle = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count();
    if (idle < 25 || idle > 1000) { return 3; }
    // 先记录后发布，即使调用wait前通知已经合并也不能睡满期限。
    for (uint32_t turn = 0; turn < 200; ++turn)
    {
        const uint64_t observed = LibuvTcp::hostLoopBegin(loop);
        LibuvTcp::hostLoopWake(loop);
        if (LibuvTcp::hostLoopWait(loop, observed, 500) != 0) { return 4; }
        LibuvTcp::hostLoopWait(loop, LibuvTcp::hostLoopBegin(loop), 0);
    }
    const uint64_t observed = LibuvTcp::hostLoopBegin(loop);
    const auto waking = std::chrono::steady_clock::now();
    std::thread producer([loop]()
    {
        // 输入无返回值；延迟模拟等待开始后另一线程发布工作。
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        LibuvTcp::hostLoopWake(loop);
    });
    const int32_t waited = LibuvTcp::hostLoopWait(loop, observed, 1000);
    producer.join();
    const auto wakeMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - waking).count();
    if (waited != 0 || wakeMs > 500) { return 5; }
    void* previous = LibuvTcp::hostLoopExchange(loop);
    NetHostExecutionTest listener;
    NetHostExecutionTest client;
    listener.m_loop = client.m_loop = loop;
    void* server = nullptr;
    void* sender = nullptr;
    uint64_t peer = 0;
    if (LibuvTcp::hostCreate(nullptr, &listener, connected, data, writable, closed, &server) != 0 ||
        LibuvTcp::hostCreate(nullptr, &client, connected, data, writable, closed, &sender) != 0 ||
        LibuvTcp::hostListen(server, "127.0.0.1", 0, 8) != 0 ||
        LibuvTcp::hostConnect(sender, "127.0.0.1", LibuvTcp::hostLocalPort(server), &peer) != 0) { return 6; }
    for (uint32_t turn = 0; turn < 100 && (listener.m_peer == 0 || client.m_peer == 0); ++turn)
    {
        LibuvTcp::hostLoopWait(loop, LibuvTcp::hostLoopBegin(loop), 50);
    }
    if (listener.m_peer == 0 || client.m_peer == 0 || listener.m_failed || client.m_failed) { return 7; }
    if (LibuvTcp::post(sender, peer, reinterpret_cast<const uint8_t*>("123456"), 6) != 0) { return 8; }
    for (uint32_t turn = 0; turn < 100 && listener.m_received != 6; ++turn)
    {
        LibuvTcp::hostLoopWait(loop, LibuvTcp::hostLoopBegin(loop), 50);
    }
    if (listener.m_received != 6 || listener.m_failed) { return 9; }
    // 另一个监听仍活动时，销毁客户端必须立即收敛，而不是等待整个共同循环退出。
    if (LibuvTcp::hostDestroy(sender) != 0 || client.m_closed != 1 || LibuvTcp::hostLoopDestroy(loop) >= 0) { return 10; }
    if (LibuvTcp::hostDestroy(server) != 0 || listener.m_closed != 1) { return 11; }
    LibuvTcp::hostLoopExchange(previous);
    if (LibuvTcp::hostLoopDestroy(loop) != 0) { return 12; }
    std::printf("EVENT_LOOP_PASS idleMs=%lld wakeMs=%lld wakeBeforeWait=200 sharedChannels=2\n",
        static_cast<long long>(idle), static_cast<long long>(wakeMs));
    return 0;
}

#ifdef NET_HOST_EXECUTION_STANDALONE
int main()
{
    const int result = NetHostExecutionTest::run();
    std::printf("EVENT_LOOP_RESULT %d\n", result);
    return result;
}
#endif

void NetHostExecutionTest::check()
{
    if (run() != 0) { throw std::runtime_error("shared host loop"); }
#ifndef NET_HOST_EXECUTION_STANDALONE
    NetExecution execution;
    NetExecutionScope scope(execution);
    execution.begin();
    // 不同起点时钟只传递剩余量；多个期限取最早值，不被后来的长期限覆盖。
    NetExecution::deadline(10005, 10000);
    NetExecution::deadline(10900, 10000);
    const auto started = std::chrono::steady_clock::now();
    if (execution.wait(1000) != NI_OK || std::chrono::steady_clock::now() - started > std::chrono::milliseconds(250))
    {
        throw std::runtime_error("short protocol deadline exceeded by idle wait");
    }
    execution.begin();
    NetExecution::deadline(9, 10);
    const auto expired = std::chrono::steady_clock::now();
    if (execution.wait(1000) != NI_OK || std::chrono::steady_clock::now() - expired > std::chrono::milliseconds(250))
    {
        throw std::runtime_error("expired protocol deadline blocked");
    }
    execution.begin();
    NetExecution::progress();
    const auto progressed = std::chrono::steady_clock::now();
    if (execution.wait(1000) != NI_OK || std::chrono::steady_clock::now() - progressed > std::chrono::milliseconds(250))
    {
        throw std::runtime_error("owner progress unexpectedly blocked");
    }
    execution.begin();
    const auto reset = std::chrono::steady_clock::now();
    if (execution.wait(40) != NI_OK || std::chrono::steady_clock::now() - reset < std::chrono::milliseconds(25))
    {
        throw std::runtime_error("completed deadline leaked into next idle turn");
    }
    std::printf("EVENT_DEADLINE_PASS earliest=5 expired=0 reset=40\n");
#endif
}