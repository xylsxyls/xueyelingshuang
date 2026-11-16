#include "NetIpcAdapterTest.h"
#include "NetIpcAdapterFixture.h"
#include "NetSenderTestAccess.h"
#include "NetSender/NetSenderLocalHub.h"
#include "NetSender/NetSenderLocalState.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>

/** 核对独立预期，失败交给runner报告
@param [in] condition 预期条件
@param [in] reason 失败原因
*/
static void IpcAdapterCheck(bool condition, const char* reason)
{
    if (!condition)
    {
        throw std::runtime_error(reason);
    }
}

void NetIpcAdapterTest::capacity()
{
    NetIpcAdapterFixture test;
    test.open(2);
    IpcAdapterCheck(test.m_sender.trySend(nullptr, 1, 0) == NET_SENDER_IPC_INVALID &&
        test.m_sender.trySend(nullptr, 0, -1) == NET_SENDER_IPC_INVALID, "IPC input validation");
    uint8_t bytes[4085] = {};
    IpcAdapterCheck(test.m_sender.trySend(bytes, sizeof(bytes), 0) == NET_SENDER_IPC_TOO_LARGE, "IPC oversized body");
    IpcAdapterCheck(test.m_sender.trySend(nullptr, 0, 0) == NET_SENDER_IPC_ACCEPTED &&
        test.m_sender.trySend(nullptr, 0, 0) == NET_SENDER_IPC_ACCEPTED, "IPC both slots committed");
    const auto started = std::chrono::steady_clock::now();
    for (uint32_t index = 0; index < 512; ++index)
    {
        IpcAdapterCheck(test.m_sender.trySend(nullptr, 0, 0) == NET_SENDER_IPC_CAPACITY, "IPC full is capacity, not busy or disconnect");
    }
    IpcAdapterCheck(std::chrono::steady_clock::now() - started < std::chrono::seconds(2), "IPC full must not block owner");
    IpcAdapterCheck(test.m_receiver.pump(test.m_packets, test.m_delivered, test.m_full) == 2, "IPC drain exact count");
    test.checkPacket(nullptr, 0, 0);
    test.checkPacket(nullptr, 0, 0);
    IpcAdapterCheck(test.m_packets.empty(), "IPC retry did not duplicate");
    IpcAdapterCheck(test.m_sender.trySend(bytes, 4084, 42) == NET_SENDER_IPC_ACCEPTED &&
        test.m_receiver.pump(test.m_packets, test.m_delivered, test.m_full) == 1, "IPC resumes after capacity release");
    test.checkPacket(bytes, 4084, 42);
    IpcAdapterCheck(NetSenderIpcChannel::retryable(NET_SENDER_IPC_BUSY) &&
        NetSenderIpcChannel::retryable(NET_SENDER_IPC_CAPACITY) && NetSenderIpcChannel::retryable(NET_SENDER_IPC_RESOURCE) &&
        !NetSenderIpcChannel::retryable(NET_SENDER_IPC_NO_MEMORY) && !NetSenderIpcChannel::retryable(NET_SENDER_IPC_UNAVAILABLE) &&
        !NetSenderIpcChannel::retryable(NET_SENDER_IPC_ACCEPTED) && !NetSenderIpcChannel::retryable(-999), "IPC retry classification");
    test.m_receiver.stop();
    IpcAdapterCheck(test.m_sender.trySend(nullptr, 0, 0) == NET_SENDER_IPC_UNAVAILABLE, "IPC stopped receiver");
    test.m_sender.close();
    test.m_receiver.close();
    const int32_t pid = CSystem::currentProcessPid();
    SharedMemory legacy("ProcessAreaAssign_" + std::to_string(pid), 20, false);
    IpcAdapterCheck(test.m_sender.open(pid, pid, 100) == NET_SENDER_IPC_UNAVAILABLE, "IPC legacy receiver not mistaken for current protocol");
}

void NetIpcAdapterTest::publication()
{
    NetIpcAdapterFixture test;
    test.open(1);
    const uint8_t bytes[] = {0, 255, 128, 7, 0};
    IpcAdapterCheck(test.m_sender.trySend(bytes, sizeof(bytes), 42) == NET_SENDER_IPC_ACCEPTED, "IPC binary publication");
    IpcAdapterCheck(test.m_sender.pendingNotifications() == 0 && test.m_sender.flushNotifications() == NET_SENDER_IPC_ACCEPTED,
        "IPC notification never duplicates payload");
    IpcAdapterCheck(test.m_sender.trySend(nullptr, 0, 0) == NET_SENDER_IPC_CAPACITY, "IPC committed block protected");
    test.m_sender.close();
    IpcAdapterCheck(test.m_receiver.pump(test.m_packets, test.m_delivered, test.m_full) == 1, "IPC close preserves committed body");
    test.checkPacket(bytes, sizeof(bytes), 42);
    IpcAdapterCheck(test.m_packets.empty(), "IPC exactly one delivered body");
    const int32_t pid = CSystem::currentProcessPid();
    IpcAdapterCheck(test.m_sender.open(pid, pid, 4084) == NET_SENDER_IPC_ACCEPTED, "IPC reopen");
    int32_t sent = NET_SENDER_IPC_RESOURCE;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (NetSenderIpcChannel::retryable(sent) && std::chrono::steady_clock::now() < deadline)
    {
        test.m_receiver.pump(test.m_packets, test.m_delivered, test.m_full);
        sent = test.m_sender.trySend(nullptr, 0, 0);
    }
    IpcAdapterCheck(sent == NET_SENDER_IPC_ACCEPTED &&
        test.m_receiver.pump(test.m_packets, test.m_delivered, test.m_full) == 1, "IPC reopen exact publication");
    test.checkPacket(nullptr, 0, 0);
    IpcAdapterCheck(test.m_packets.empty(), "IPC reopened stream has no stale data");
}

void NetIpcAdapterTest::lifecycle()
{
    ProcessWork& work = ProcessWork::instance();
    NetSenderLocalHub& hub = NetSenderLocalHub::instance();
    NetSenderLocalInbox first;
    NetSenderLocalInbox second;
    uint32_t leases = 0;
    std::vector<std::unique_ptr<NetSenderLocalInbox>> boxes;
    bool external = false;
    IpcAdapterCheck(work.receiveBufferCount() == 0, "hub test requires an idle test receiver");
    try
    {
        // 只attach曾经会偷偷登记Hub回调，detach又忘记登记，后续重试造成重复回调。
        IpcAdapterCheck(hub.attach(&first) == NI_INVALID_STATE && hub.attach(nullptr) == NI_INVALID_ARGUMENT,
            "unowned attach registered a ProcessWork callback");
        IpcAdapterCheck(hub.prepareExternal() == NI_OK, "hub external preparation");
        external = true;
        IpcAdapterCheck(hub.prepareExternal() == NI_OK && hub.acquire() == NI_INVALID_STATE && hub.attach(&first) == NI_INVALID_STATE,
            "hub lease accepted before receiver readiness");
        IpcAdapterCheck(work.initReceive(kNetSenderLocalReceiveBytes, 4), "hub external receiver startup");
        IpcAdapterCheck(hub.acquire() == NI_OK, "hub first external lease");
        ++leases;
        IpcAdapterCheck(hub.acquire() == NI_OK, "hub second external lease");
        ++leases;
        IpcAdapterCheck(hub.attach(&first) == NI_OK && hub.attach(&first) == NI_OK && hub.attach(&second) == NI_OK,
            "hub repeated attach or shared targets");
        hub.release();
        --leases;
        hub.detach(&first);
        IpcAdapterCheck(work.receiveBufferCount() == 4 && hub.externalStopped() == NI_INVALID_STATE,
            "hub released another external client");
        hub.release();
        --leases;
        hub.detach(&second);
        IpcAdapterCheck(work.receiveBufferCount() == 4 && hub.externalStopped() == NI_INVALID_STATE,
            "hub stopped a host-owned receiver");
        work.uninitReceive();
        IpcAdapterCheck(hub.externalStopped() == NI_OK, "hub external registration did not return to idle");
        external = false;
        IpcAdapterCheck(hub.attach(&first) == NI_INVALID_STATE, "hub retained a stale registration");
        IpcAdapterCheck(hub.acquire() == NI_OK, "hub managed startup after external shutdown");
        ++leases;
        IpcAdapterCheck(hub.attach(&first) == NI_OK && work.receivePayloadCapacity() == 16440, "hub managed receiver capacity");
        hub.release();
        --leases;
        IpcAdapterCheck(work.receiveBufferCount() == 64, "hub stopped while a borrowed inbox was still attached");
        hub.detach(&first);
        IpcAdapterCheck(work.receiveBufferCount() == 0, "last detach did not stop managed receiver");
        hub.release();
        IpcAdapterCheck(work.receiveBufferCount() == 0, "extra release restarted receiver");
        for (uint32_t index = 0; index < 1024; ++index)
        {
            IpcAdapterCheck(hub.acquire() == NI_OK, "hub lease cannot grow past old 64");
            ++leases;
        }
        // 租约仅用于生命期计数，不再设置64个业务门槛。
        IpcAdapterCheck(hub.acquire() == NI_OK, "hub lease growth");
        ++leases;
        while (leases != 0)
        {
            hub.release();
            --leases;
        }
        IpcAdapterCheck(work.receiveBufferCount() == 0, "hub final lease leaked receiver tasks");
        LockFreeHashMapPoolConfig config;
        config.initialCapacity = 2;
        config.maximumCapacity = 256;
        config.directoryCapacity = 128;
        IpcAdapterCheck(hub.configurePool(config) == NI_OK, "hub configure actual pool");
        IpcAdapterCheck(hub.acquire() == NI_OK, "hub growth lease");
        ++leases;
        IpcAdapterCheck(hub.configurePool(config) == NI_INVALID_STATE, "hub pool changed under a lease");
        for (uint32_t index = 0; index < 256; ++index)
        {
            boxes.push_back(std::unique_ptr<NetSenderLocalInbox>(new NetSenderLocalInbox));
            IpcAdapterCheck(hub.attach(boxes.back().get()) == NI_OK, "hub actual pool cannot exceed old 64 boxes");
        }
        IpcAdapterCheck(hub.attach(&first) == NI_LIMIT_EXCEEDED, "hub actual slot capacity not enforced");
        hub.detach(boxes.back().get());
        IpcAdapterCheck(hub.attach(&first) == NI_OK, "hub did not reuse a returned slot");
        hub.detach(&first);
        for (const auto& box : boxes) { hub.detach(box.get()); }
        boxes.clear();
        hub.release();
        --leases;
        IpcAdapterCheck(work.receiveBufferCount() == 0, "hub growth leaked receiver tasks");
        config.initialCapacity = 128;
        config.maximumCapacity = 65536;
        IpcAdapterCheck(hub.configurePool(config) == NI_OK, "hub restore ordinary storage");
    }
    catch (...)
    {
        for (const auto& box : boxes) { hub.detach(box.get()); }
        hub.detach(&first);
        hub.detach(&second);
        while (leases != 0)
        {
            hub.release();
            --leases;
        }
        work.uninitReceive();
        if (external)
        {
            hub.externalStopped();
        }
        throw;
    }
}

void NetIpcAdapterTest::phase()
{
    NetSenderLocalPeer lost;
    IpcAdapterCheck(lost.m_phase == NET_LOCAL_ACTIVE && lost.m_drainSince == 0, "IPC phase monotonic");
    lost.drain(0, false);
    IpcAdapterCheck(lost.m_phase == NET_LOCAL_DRAINING && lost.m_drainSince == 0, "IPC phase monotonic");
    lost.drain(4999, false);
    IpcAdapterCheck(lost.m_phase == NET_LOCAL_DRAINING && lost.m_drainSince == 0, "IPC phase monotonic");
    lost.drain(4999, true);
    IpcAdapterCheck(lost.m_phase == NET_LOCAL_ENDED && lost.m_drainSince == 0, "IPC phase monotonic");
    lost.drain(5000, false);
    lost.drain(5001, true);
    IpcAdapterCheck(lost.m_phase == NET_LOCAL_ENDED && lost.m_drainSince == 0, "IPC phase monotonic");
    lost.m_phase = NET_LOCAL_CLOSED;
    lost.drain(6000, true);
    IpcAdapterCheck(lost.m_phase == NET_LOCAL_CLOSED && lost.m_drainSince == 0, "IPC phase monotonic");
    NetSenderLocalPeer ended;
    ended.drain(17, true);
    IpcAdapterCheck(ended.m_phase == NET_LOCAL_ENDED && ended.m_drainSince == 17, "IPC phase monotonic");
    ended.drain(9000, true);
    ended.drain(9001, false);
    IpcAdapterCheck(ended.m_phase == NET_LOCAL_ENDED && ended.m_drainSince == 17, "IPC phase monotonic");
}