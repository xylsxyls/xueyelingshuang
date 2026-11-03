#define NET_WORK_HOST_NO_HEAP
#include <LibuvTcp/LibuvTcpAPI.h>
#include "NetWorkConcurrencyTestCases.h"
#include <NetWork/NetWorkAPI.h>
#include <NetSender/NetSenderAPI.h>
#include <test/certs.h>
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "NetWorkTestCases.h"
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_WORK_HOST_NO_HEAP)
#include <crtdbg.h>
static int32_t s_bridgeRejectAfter = -1;
static uint32_t s_bridgeAllocations = 0;

/** 隔离main中拒绝指定普通堆分配，统计无分配控制路径
@param [in] operation CRT分配操作，其他参数沿用CRT回调原型
@param [in] pointer CRT传入的堆块，只在钩子返回前借用；本用例未使用时忽略
@param [in] size CRT请求的字节数，是否参与注入判定由本用例决定
@param [in] block CRT堆块类别，只对普通业务堆块注入
@param [in] request CRT分配序号，本用例不使用
@param [in] filename CRT传入的来源文件名，本用例不读取
@param [in] line 来源代码行号，本用例不使用
@return 非零允许分配，0持续拒绝
*/
static int __cdecl BridgeMemoryHook(int operation, void* pointer, size_t size, int block, long request, const unsigned char* filename, int line)
{
    (void)pointer;
    (void)size;
    (void)request;
    (void)filename;
    (void)line;
    if (block == _NORMAL_BLOCK && (operation == _HOOK_ALLOC || operation == _HOOK_REALLOC))
    {
        ++s_bridgeAllocations;
        if (s_bridgeRejectAfter == 0) { return 0; }
        if (s_bridgeRejectAfter > 0) { --s_bridgeRejectAfter; }
    }
    return 1;
}
#endif

#include "NetWork/NetWorkAPI.h"
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

NetWorkHostBridgeTest::NetWorkHostBridgeTest() :
m_bridge(nullptr), m_target(nullptr), m_connected(nullptr), m_data(nullptr), m_writable(nullptr), m_closed(nullptr),
m_owner(CSystem::SystemThreadId()), m_connections(0), m_lastConnected(0), m_checkConnectionOrder(false), m_inputs(0), m_writes(0), m_closes(0), m_stops(0), m_allocated(0), m_peerInputs(), m_peerCloses(), m_reason(0), m_pause(true), m_checkPayload(true), m_sequence(false), m_expected(), m_producersDone(0), m_stopped(false), m_throwPoll(false), m_bad(false), m_done(false)
{

}

NetHostFunctions NetWorkHostBridgeTest::functions()
{
    NetHostFunctions value = {};
    value.structSize = sizeof(value); value.version = 1; value.context = this;
    value.create = create; value.connect = connect; value.poll = poll; value.write = write; value.pauseRead = pause;
    value.close = close; value.shutdown = shutdown; value.destroy = operation; value.localPort = query; value.peerCount = query;
    return value;
}

int32_t NetWorkHostBridgeTest::create(void* provider, void* observer, NetHostConnected connected, NetHostData data, NetHostWritable writable, NetHostClosed closed, void** instance)
{
    NetWorkHostBridgeTest* test = static_cast<NetWorkHostBridgeTest*>(provider);
    test->m_target = observer; test->m_connected = connected; test->m_data = data; test->m_writable = writable; test->m_closed = closed;
    *instance = test; return 0;
}

int32_t NetWorkHostBridgeTest::operation(void* instance)
{
    (void)instance; return 0;
}

int32_t NetWorkHostBridgeTest::poll(void* instance)
{
    NetWorkHostBridgeTest* test = static_cast<NetWorkHostBridgeTest*>(instance);
    if (test->m_throwPoll) { throw std::bad_alloc(); }
    return test->m_stopped ? 0 : 1;
}

int32_t NetWorkHostBridgeTest::shutdown(void* instance)
{
    NetWorkHostBridgeTest* test = static_cast<NetWorkHostBridgeTest*>(instance); test->m_stopped = true; ++test->m_stops; return 0;
}

int32_t NetWorkHostBridgeTest::close(void* instance, uint64_t peer, int32_t reason)
{
    NetWorkHostBridgeTest* test = static_cast<NetWorkHostBridgeTest*>(instance); test->m_closed(test->m_target, peer, reason); return 0;
}

int32_t NetWorkHostBridgeTest::pause(void* instance, uint64_t peer, int32_t paused)
{
    (void)instance; (void)peer; (void)paused; return 0;
}

int32_t NetWorkHostBridgeTest::connect(void* instance, const char* address, uint32_t port, uint64_t* peer)
{
    (void)address; (void)port; *peer = ++static_cast<NetWorkHostBridgeTest*>(instance)->m_allocated; return 0;
}

int32_t NetWorkHostBridgeTest::write(void* instance, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    (void)instance; (void)peer; (void)bytes; (void)size; return 0;
}

uint32_t NetWorkHostBridgeTest::query(void* instance)
{
    (void)instance; return 0;
}

void NetWorkHostBridgeTest::connected(void* context, uint64_t peer)
{
    NetWorkHostBridgeTest* test = static_cast<NetWorkHostBridgeTest*>(context);
    ++test->m_connections;
    if (test->m_checkConnectionOrder && peer <= test->m_lastConnected)
    {
        test->m_bad = true;
    }
    test->m_lastConnected = peer;
    test->m_bad = test->m_bad || CSystem::SystemThreadId() != test->m_owner || NetWorkHostBridge::destroy(test->m_bridge) != -1;
}

void NetWorkHostBridgeTest::data(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    NetWorkHostBridgeTest* test = static_cast<NetWorkHostBridgeTest*>(context); ++test->m_inputs;
    test->m_bad = test->m_bad || CSystem::SystemThreadId() != test->m_owner ||
        (test->m_checkPayload && (size != 1 || bytes == nullptr || bytes[0] != 7));
    if (peer >= 1 && peer <= 2) { ++test->m_peerInputs[peer - 1]; }
    if (test->m_sequence)
    {
        uint32_t record[2] = {};
        if (size != sizeof(record) || bytes == nullptr) { test->m_bad = true; }
        else
        {
            std::memcpy(record, bytes, sizeof(record));
            if (record[0] >= 4 || record[1] != test->m_expected[record[0]]++) { test->m_bad = true; }
        }
    }
    if (test->m_pause && peer == 1) { NetWorkHostBridge::pauseRead(test->m_bridge, peer, 1); }
}

void NetWorkHostBridgeTest::writable(void* context, uint64_t peer)
{
    (void)peer; ++static_cast<NetWorkHostBridgeTest*>(context)->m_writes;
}

void NetWorkHostBridgeTest::closed(void* context, uint64_t peer, int32_t reason)
{
    NetWorkHostBridgeTest* test = static_cast<NetWorkHostBridgeTest*>(context); ++test->m_closes;
    test->m_reason = reason;
    if (peer >= 1 && peer <= 2) { ++test->m_peerCloses[peer - 1]; }
    test->m_bad = test->m_bad || CSystem::SystemThreadId() != test->m_owner ||
        (test->m_checkConnectionOrder && peer > test->m_lastConnected);
}

NetWorkHostBridgeTestTask::NetWorkHostBridgeTestTask(NetWorkHostBridgeTest* test) : m_test(test)
{

}

void NetWorkHostBridgeTestTask::DoTask()
{
    m_test->m_connected(m_test->m_target, 1); m_test->m_connected(m_test->m_target, 2);
    uint8_t bytes = 7;
    m_test->m_data(m_test->m_target, 1, &bytes, 1); m_test->m_data(m_test->m_target, 1, &bytes, 1); m_test->m_data(m_test->m_target, 2, &bytes, 1);
    for (uint32_t i = 0; i < 100; ++i) { m_test->m_writable(m_test->m_target, 2); }
    m_test->m_closed(m_test->m_target, 1, 0); m_test->m_closed(m_test->m_target, 2, 0);
    bytes = 99; m_test->m_done = true;
}

NetWorkHostBridgeSequenceTask::NetWorkHostBridgeSequenceTask(NetWorkHostBridgeTest* test, uint32_t producer) :
m_test(test), m_producer(producer)
{

}

void NetWorkHostBridgeSequenceTask::DoTask()
{
    for (uint32_t i = 0; i < 400; ++i)
    {
        const uint32_t record[2] = {m_producer, i};
        m_test->m_data(m_test->m_target, 1, reinterpret_cast<const uint8_t*>(record), sizeof(record));
    }
    m_test->m_producersDone.fetch_add(1);
}

int32_t NetWorkHostBridgeTest::audit()
{
    uint32_t checks = 0;
#define HOST_CHECK(condition) { ++checks; if (!(condition)) { std::printf("FAIL line=%u %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
    // 第一条连接退役后，轮询游标位于槽1；第二、三条连接复用槽0、1。
    // 新连接通告仍须2在3之前，不能把轮询公平性变成代次倒序。
    NetWorkHostBridgeTest connectionOrder;
    connectionOrder.m_checkConnectionOrder = true;
    connectionOrder.m_pause = false;
    NetHostFunctions host = connectionOrder.functions();
    HOST_CHECK(NetWorkHostBridge::create(&host, &connectionOrder, connectionOrder.connected, connectionOrder.data,
        connectionOrder.writable, connectionOrder.closed, &connectionOrder.m_bridge) == 0);
    connectionOrder.m_connected(connectionOrder.m_target, 1);
    HOST_CHECK(NetWorkHostBridge::poll(connectionOrder.m_bridge) == 1 && connectionOrder.m_connections == 1);
    connectionOrder.m_closed(connectionOrder.m_target, 1, NI_OK);
    HOST_CHECK(NetWorkHostBridge::poll(connectionOrder.m_bridge) == 1 && connectionOrder.m_closes == 1);
    connectionOrder.m_connected(connectionOrder.m_target, 2);
    connectionOrder.m_connected(connectionOrder.m_target, 3);
    HOST_CHECK(NetWorkHostBridge::poll(connectionOrder.m_bridge) == 1 && connectionOrder.m_connections == 3 &&
        connectionOrder.m_lastConnected == 3 && !connectionOrder.m_bad);
    connectionOrder.m_closed(connectionOrder.m_target, 2, NI_OK);
    connectionOrder.m_closed(connectionOrder.m_target, 3, NI_OK);
    HOST_CHECK(NetWorkHostBridge::poll(connectionOrder.m_bridge) == 1 && connectionOrder.m_closes == 3);
    HOST_CHECK(NetWorkHostBridge::shutdown(connectionOrder.m_bridge) == 0 &&
        NetWorkHostBridge::destroy(connectionOrder.m_bridge) == 0);
    NetWorkHostBridgeTest test;
    host = test.functions();
    HOST_CHECK(NetWorkHostBridge::create(&host, &test, test.connected, test.data, test.writable, test.closed, &test.m_bridge) == 0);
    const uint32_t thread = CTaskThreadManager::Instance().Init();
    CTaskThreadManager::Instance().GetThreadInterface(thread)->PostTask(std::shared_ptr<CTask>(new NetWorkHostBridgeTestTask(&test)));
    const uint64_t deadline = GetTickCount64() + 5000;
    while (!test.m_done.load() && GetTickCount64() < deadline) { CSystem::sleepPrecise(1); }
    HOST_CHECK(test.m_done.load());
    CTaskThreadManager::Instance().Uninit(thread);
    HOST_CHECK(test.m_connections == 0 && test.m_inputs == 0 && test.m_closes == 0);
    HOST_CHECK(NetWorkHostBridge::poll(test.m_bridge) == 1);
    HOST_CHECK(test.m_connections == 2 && test.m_inputs == 2 && test.m_writes == 1 && test.m_closes == 1 && !test.m_bad);
    test.m_pause = false;
    HOST_CHECK(NetWorkHostBridge::pauseRead(test.m_bridge, 1, 0) == 0);
    HOST_CHECK(NetWorkHostBridge::poll(test.m_bridge) == 1 && test.m_inputs == 3 && test.m_closes == 2 && !test.m_bad);
    HOST_CHECK(NetWorkHostBridge::shutdown(test.m_bridge) == 0 && test.m_stops == 1);
    HOST_CHECK(NetWorkHostBridge::destroy(test.m_bridge) == 0);
    // 满4MiB数据之后终态仍有独立事件位置，不会被丢掉。
    NetWorkHostBridgeTest full; full.m_pause = false; full.m_checkPayload = false; host = full.functions();
    HOST_CHECK(NetWorkHostBridge::create(&host, &full, full.connected, full.data, full.writable, full.closed, &full.m_bridge) == 0);
    full.m_connected(full.m_target, 1);
    uint8_t bytes[16384] = {};
    for (uint32_t i = 0; i < 256; ++i) { full.m_data(full.m_target, 1, bytes, sizeof(bytes)); }
    full.m_closed(full.m_target, 1, 0);
    // 已接纳EOF之后的迟到数据忽略，不影响已排队尾部与其他连接。
    full.m_data(full.m_target, 1, bytes, 1);
    for (uint32_t i = 0; i < 3; ++i) { NetWorkHostBridge::poll(full.m_bridge); }
    HOST_CHECK(full.m_inputs == 256 && full.m_closes == 1 && full.m_stops == 0 && !full.m_bad);
    HOST_CHECK(NetWorkHostBridge::shutdown(full.m_bridge) == 0);
    HOST_CHECK(NetWorkHostBridge::destroy(full.m_bridge) == 0);
    // shutdown锁外转交事件时也处于受保护回调段，不能在其中释放bridge。
    NetWorkHostBridgeTest stopping; host = stopping.functions();
    HOST_CHECK(NetWorkHostBridge::create(&host, &stopping, stopping.connected, stopping.data, stopping.writable, stopping.closed, &stopping.m_bridge) == 0);
    stopping.m_connected(stopping.m_target, 1);
    stopping.m_closed(stopping.m_target, 1, 0);
    HOST_CHECK(NetWorkHostBridge::shutdown(stopping.m_bridge) == 0 && stopping.m_connections == 1 && stopping.m_closes == 1 && !stopping.m_bad);
    HOST_CHECK(NetWorkHostBridge::destroy(stopping.m_bridge) == 0);
    // 宿主C++错误不能永久遗留polling，外层捕获后仍可收敛和销毁。
    NetWorkHostBridgeTest throwing; host = throwing.functions();
    HOST_CHECK(NetWorkHostBridge::create(&host, &throwing, throwing.connected, throwing.data, throwing.writable, throwing.closed, &throwing.m_bridge) == 0);
    throwing.m_throwPoll = true;
    HOST_CHECK(NetWorkHostBridge::poll(throwing.m_bridge) == -1);
    HOST_CHECK(NetWorkHostBridge::shutdown(throwing.m_bridge) == 0);
    HOST_CHECK(NetWorkHostBridge::destroy(throwing.m_bridge) == 0);
    // 所有2048个槽都被暂停连接占用，两个连接的EOF仍可接纳。
    NetWorkHostBridgeTest slots; slots.m_pause = false; host = slots.functions();
    HOST_CHECK(NetWorkHostBridge::create(&host, &slots, slots.connected, slots.data, slots.writable, slots.closed, &slots.m_bridge) == 0);
    slots.m_connected(slots.m_target, 1); slots.m_connected(slots.m_target, 2);
    NetWorkHostBridge::poll(slots.m_bridge);
    NetWorkHostBridge::pauseRead(slots.m_bridge, 1, 1);
    uint8_t seven = 7;
    for (uint32_t i = 0; i < 2048; ++i) { slots.m_data(slots.m_target, 1, &seven, 1); }
    slots.m_closed(slots.m_target, 1, NI_OK); slots.m_closed(slots.m_target, 2, NI_OK);
    HOST_CHECK(NetWorkHostBridge::poll(slots.m_bridge) >= 0 && slots.m_peerCloses[1] == 1 && slots.m_peerCloses[0] == 0 && slots.m_stops == 0);
    HOST_CHECK(NetWorkHostBridge::pauseRead(slots.m_bridge, 1, 0) == 0);
    for (uint32_t i = 0; i < 9; ++i) { NetWorkHostBridge::poll(slots.m_bridge); }
    HOST_CHECK(slots.m_peerInputs[0] == 2048 && slots.m_peerCloses[0] == 1 && !slots.m_bad);
    for (uint32_t i = 0; i < 4096; ++i)
    {
        slots.m_data(slots.m_target, 1, &seven, 1);
        slots.m_writable(slots.m_target, 1);
        slots.m_closed(slots.m_target, 1, NI_OK);
    }
    HOST_CHECK(NetWorkHostBridge::poll(slots.m_bridge) >= 0 && slots.m_closes == 2 && slots.m_stops == 0);
    NetWorkHostBridge::shutdown(slots.m_bridge);
    HOST_CHECK(NetWorkHostBridge::destroy(slots.m_bridge) == 0);
    // 两种容量上限分别覆盖通告前后过载；关闭不能先于新连接通告。
    for (uint32_t mode = 0; mode < 4; ++mode)
    {
        NetWorkHostBridgeTest overflow; overflow.m_pause = false; overflow.m_checkConnectionOrder = true; host = overflow.functions();
        HOST_CHECK(NetWorkHostBridge::create(&host, &overflow, overflow.connected, overflow.data, overflow.writable, overflow.closed, &overflow.m_bridge) == 0);
        overflow.m_connected(overflow.m_target, 1); overflow.m_connected(overflow.m_target, 2);
        if (mode < 2) { NetWorkHostBridge::poll(overflow.m_bridge); }
        for (uint32_t i = 0; i < (mode % 2 == 0 ? 2048U : 256U); ++i)
        {
            overflow.m_data(overflow.m_target, 1, mode % 2 == 0 ? &seven : bytes, mode % 2 == 0 ? 1 : sizeof(bytes));
        }
        overflow.m_data(overflow.m_target, 1, &seven, 1);
        overflow.m_data(overflow.m_target, 2, &seven, 1);
        HOST_CHECK(NetWorkHostBridge::poll(overflow.m_bridge) >= 0 && overflow.m_peerCloses[0] == 1 && overflow.m_peerCloses[1] == 0 && overflow.m_peerInputs[1] == 1 && overflow.m_reason == NI_LIMIT_EXCEEDED && overflow.m_connections == 2 && overflow.m_stops == 0 && !overflow.m_bad);
        overflow.m_data(overflow.m_target, 2, &seven, 1);
        NetWorkHostBridge::poll(overflow.m_bridge);
        HOST_CHECK(overflow.m_peerInputs[1] == 2 && overflow.m_closes == 1 && !overflow.m_bad);
        NetWorkHostBridge::shutdown(overflow.m_bridge);
        HOST_CHECK(NetWorkHostBridge::destroy(overflow.m_bridge) == 0);
    }
    // 主动connect与事件入口共享256个连接槽，满时不调用原始宿主。
    NetWorkHostBridgeTest limit; limit.m_pause = false; host = limit.functions();
    HOST_CHECK(NetWorkHostBridge::create(&host, &limit, limit.connected, limit.data, limit.writable, limit.closed, &limit.m_bridge) == 0);
    uint64_t peer = 0;
    for (uint32_t i = 0; i < 256; ++i)
    {
        HOST_CHECK(NetWorkHostBridge::connect(limit.m_bridge, "127.0.0.1", 1, &peer) == 0 && peer == i + 1);
    }
    HOST_CHECK(NetWorkHostBridge::connect(limit.m_bridge, "127.0.0.1", 1, &peer) < 0 && peer == 0 && limit.m_allocated == 256);
    for (uint32_t i = 0; i < 256; ++i) { limit.m_closed(limit.m_target, i + 1, NI_OK); }
    NetWorkHostBridge::poll(limit.m_bridge);
    HOST_CHECK(limit.m_closes == 256 && limit.m_stops == 0);
    HOST_CHECK(NetWorkHostBridge::connect(limit.m_bridge, "127.0.0.1", 1, &peer) == 0 && peer == 257);
    NetWorkHostBridge::shutdown(limit.m_bridge);
    HOST_CHECK(NetWorkHostBridge::destroy(limit.m_bridge) == 0);
    NetWorkHostBridgeTest ordered; ordered.m_pause = false; ordered.m_checkPayload = false; ordered.m_sequence = true;
    host = ordered.functions();
    HOST_CHECK(NetWorkHostBridge::create(&host, &ordered, ordered.connected, ordered.data, ordered.writable, ordered.closed, &ordered.m_bridge) == 0);
    ordered.m_connected(ordered.m_target, 1);
    uint32_t producers[4] = {};
    for (uint32_t i = 0; i < 4; ++i)
    {
        producers[i] = CTaskThreadManager::Instance().Init();
        CTaskThreadManager::Instance().GetThreadInterface(producers[i])->PostTask(std::shared_ptr<CTask>(new NetWorkHostBridgeSequenceTask(&ordered, i)));
    }
    const uint64_t orderedDeadline = GetTickCount64() + 5000;
    while (ordered.m_producersDone.load() != 4 && GetTickCount64() < orderedDeadline) { CSystem::sleepPrecise(1); }
    for (uint32_t i = 0; i < 4; ++i) { CTaskThreadManager::Instance().Uninit(producers[i]); }
    HOST_CHECK(ordered.m_producersDone.load() == 4 && ordered.m_inputs == 0);
    ordered.m_closed(ordered.m_target, 1, NI_OK);
    for (uint32_t i = 0; i < 8; ++i) { NetWorkHostBridge::poll(ordered.m_bridge); }
    HOST_CHECK(ordered.m_inputs == 1600 && ordered.m_closes == 1 && ordered.m_stops == 0 && !ordered.m_bad);
    for (uint32_t i = 0; i < 4; ++i) { HOST_CHECK(ordered.m_expected[i] == 400); }
    NetWorkHostBridge::shutdown(ordered.m_bridge);
    HOST_CHECK(NetWorkHostBridge::destroy(ordered.m_bridge) == 0);
#if defined(_WIN32) && defined(_DEBUG) && !defined(NET_WORK_HOST_NO_HEAP)
    _CrtMemState before, after;
    _CrtMemCheckpoint(&before);
    for (int32_t position = 0; position < 4; ++position)
    {
        NetWorkHostBridgeTest allocation; allocation.m_pause = false; host = allocation.functions();
        s_bridgeRejectAfter = position;
        _CRT_ALLOC_HOOK oldHook = _CrtSetAllocHook(BridgeMemoryHook);
        const int32_t result = NetWorkHostBridge::create(&host, &allocation, allocation.connected, allocation.data, allocation.writable, allocation.closed, &allocation.m_bridge);
        _CrtSetAllocHook(oldHook); s_bridgeRejectAfter = -1;
        HOST_CHECK(position == 0 ? result < 0 && allocation.m_bridge == nullptr : result == 0 && allocation.m_bridge != nullptr);
        if (allocation.m_bridge != nullptr)
        {
            NetWorkHostBridge::shutdown(allocation.m_bridge);
            HOST_CHECK(NetWorkHostBridge::destroy(allocation.m_bridge) == 0);
        }
    }
    for (int32_t position = 0; position < 3; ++position)
    {
        NetWorkHostBridgeTest allocation; allocation.m_pause = false; host = allocation.functions();
        HOST_CHECK(NetWorkHostBridge::create(&host, &allocation, allocation.connected, allocation.data, allocation.writable, allocation.closed, &allocation.m_bridge) == 0);
        allocation.m_checkConnectionOrder = true;
        allocation.m_connected(allocation.m_target, 1); allocation.m_connected(allocation.m_target, 2);
        // 未向owner通告时注入输入分配失败，仍须先连接后关闭。
        s_bridgeRejectAfter = position;
        _CRT_ALLOC_HOOK oldHook = _CrtSetAllocHook(BridgeMemoryHook);
        allocation.m_data(allocation.m_target, 1, &seven, 1);
        _CrtSetAllocHook(oldHook); s_bridgeRejectAfter = -1;
        allocation.m_data(allocation.m_target, 2, &seven, 1);
        NetWorkHostBridge::poll(allocation.m_bridge);
        HOST_CHECK(allocation.m_connections == 2 && allocation.m_peerInputs[1] == 1 && allocation.m_peerCloses[1] == 0 && allocation.m_stops == 0 && !allocation.m_bad);
        HOST_CHECK(position == 0 ? allocation.m_reason == NI_NO_MEMORY && allocation.m_peerCloses[0] == 1 && allocation.m_peerInputs[0] == 0 : allocation.m_peerCloses[0] == 0 && allocation.m_peerInputs[0] == 1);
        s_bridgeRejectAfter = 0; s_bridgeAllocations = 0;
        oldHook = _CrtSetAllocHook(BridgeMemoryHook);
        for (uint32_t i = 0; i < 10000; ++i) { allocation.m_writable(allocation.m_target, 2); }
        allocation.m_closed(allocation.m_target, 2, NI_OK);
        NetWorkHostBridge::poll(allocation.m_bridge);
        _CrtSetAllocHook(oldHook); s_bridgeRejectAfter = -1;
        HOST_CHECK(s_bridgeAllocations == 0 && allocation.m_peerCloses[1] == 1 && allocation.m_writes == 1 && !allocation.m_bad);
        NetWorkHostBridge::shutdown(allocation.m_bridge);
        HOST_CHECK(NetWorkHostBridge::destroy(allocation.m_bridge) == 0);
    }
    _CrtMemCheckpoint(&after);
    HOST_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
    std::printf("HOST_BRIDGE allocation_faults=7 control_allocations=0 heap_delta=0\n");
#endif
    std::printf("PASS NetWork native foreign-event/pause/fault checks=%u\n", checks);
    return 0;
}