#include "NetRelayTransport.h"
#include "NetPeerMacro.h"
#include "NetRelayTransportState.h"
#include <chrono>
#include <cstring>

NetRelayTransportListener::~NetRelayTransportListener()
{

}

NetRelayTransport::NetRelayTransport() :
m_state(new NetRelayTransportState())
{

}

NetRelayTransport::~NetRelayTransport()
{
    shutdown();
    delete m_state;
}

int32_t NetRelayTransport::listen(const char* address, uint32_t port, NetRelayTransportListener* listener, uint32_t maxPeers)
{
    if (m_state->m_phase != NET_RELAY_IDLE || listener == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    m_state->m_listener = listener;
    m_state->m_phase = NET_RELAY_STARTING;
    const int32_t result = m_state->m_channel.listen(address, port, m_state, maxPeers);
    m_state->m_server = result == NI_OK;
    m_state->m_phase = result == NI_OK ? NET_RELAY_ACTIVE : NET_RELAY_STOPPING;
    if (result != NI_OK)
    {
        shutdown();
    }
    return result;
}

int32_t NetRelayTransport::connect(const char* address, uint32_t port, NetRelayTransportListener* listener, uint64_t& peer)
{
    peer = 0;
    if (m_state->m_phase != NET_RELAY_IDLE || listener == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    m_state->m_listener = listener;
    m_state->m_phase = NET_RELAY_STARTING;
    int32_t result = m_state->m_channel.connect(address, port, m_state, peer);
    if (result == NI_OK)
    {
        try
        {
            std::unique_ptr<NetRelayTransportPeer> entry(new NetRelayTransportPeer(NetRelayTransportState::now()));
            m_state->m_peers.insert(std::make_pair(peer, std::move(entry)));
        }
        catch (...)
        {
            m_state->m_channel.close(peer, NI_NO_MEMORY);
            peer = 0;
            result = NI_NO_MEMORY;
        }
    }
    m_state->m_phase = result == NI_OK ? NET_RELAY_ACTIVE : NET_RELAY_STOPPING;
    if (result != NI_OK)
    {
        shutdown();
    }
    return result;
}

bool NetRelayTransport::poll()
{
    if (m_state->m_polling)
    {
        return m_state->m_phase == NET_RELAY_ACTIVE;
    }
    if (m_state->m_phase != NET_RELAY_ACTIVE)
    {
        return false;
    }
    m_state->m_polling = true;
    m_state->m_channel.poll();
    if (m_state->m_channel.status() < 0)
    {
        // 宿主已停止监听，仍须先结算EOF前已接纳的输入和终态。
        m_state->m_server = false;
    }
    for (auto entry = m_state->m_peers.begin(); entry != m_state->m_peers.end();)
    {
        const uint64_t id = entry->first;
        NetRelayTransportPeer& peer = *entry->second;
        try
        {
            m_state->pump(id, peer);
        }
        catch (...)
        {
            m_state->close(id, NI_NO_MEMORY);
        }
        if (peer.m_closed && peer.m_phase == NET_RELAY_CLOSING)
        {
            const int32_t reason = peer.m_reason;
            entry = m_state->m_peers.erase(entry);
            try
            {
                m_state->m_listener->onRelayClosed(id, reason);
            }
            catch (...)
            {
                // 先回收再通知；监听者异常不得重复关闭或带出事件循环。
            }
        }
        else
        {
            ++entry;
        }
    }
    m_state->m_polling = false;
    if (m_state->m_phase == NET_RELAY_STOPPING)
    {
        shutdown();
    }
    return m_state->m_phase == NET_RELAY_ACTIVE && (m_state->m_server || !m_state->m_peers.empty());
}

int32_t NetRelayTransport::send(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    auto found = m_state->m_peers.find(peer);
    if (m_state->m_phase != NET_RELAY_ACTIVE || found == m_state->m_peers.end() ||
        found->second->m_phase == NET_RELAY_CONNECTING || found->second->m_phase == NET_RELAY_CLOSING || found->second->m_closed)
    {
        return NI_INVALID_STATE;
    }
    return found->second->enqueue(record);
}

void NetRelayTransport::close(uint64_t peer, int32_t reason)
{
    m_state->close(peer, reason);
}

void NetRelayTransport::shutdown()
{
    if (m_state->m_phase == NET_RELAY_STOPPED)
    {
        return;
    }
    m_state->m_phase = NET_RELAY_STOPPING;
    if (m_state->m_polling)
    {
        return;
    }
    m_state->m_polling = true;
    for (auto& entry : m_state->m_peers)
    {
        m_state->close(entry.first, NI_OK);
    }
    m_state->m_channel.shutdown();
    while (!m_state->m_peers.empty())
    {
        auto entry = m_state->m_peers.begin();
        const uint64_t id = entry->first;
        const int32_t reason = entry->second->m_reason;
        m_state->m_peers.erase(entry);
        try
        {
            m_state->m_listener->onRelayClosed(id, reason);
        }
        catch (...)
        {
            // 终态已结算，继续释放其余连接。
        }
    }
    m_state->m_server = false;
    m_state->m_phase = NET_RELAY_STOPPED;
    m_state->m_polling = false;
}

uint32_t NetRelayTransport::localPort() const
{
    return m_state->m_channel.localPort();
}

int32_t NetRelayTransport::outputState(uint64_t peer) const
{
    const auto found = m_state->m_peers.find(peer);
    if (m_state->m_phase != NET_RELAY_ACTIVE || found == m_state->m_peers.end() || found->second->m_phase == NET_RELAY_CLOSING)
    {
        return NI_INVALID_STATE;
    }
    return found->second->m_dataCount + found->second->m_controlCount == 0 ? NI_OK : NI_READY;
}

// NET_RELAY_UNIT_BEGIN
//
// #include "NetPeer/NetHostSocketTest.h"
// #include "NetPeer.h"
// #include <cstring>
// #include <chrono>
// #include <cstdio>
//
// NetHostSocketTestDriver::NetHostSocketTestDriver() :
// m_observer(nullptr),
// m_connected(nullptr),
// m_data(nullptr),
// m_writable(nullptr),
// m_closed(nullptr),
// m_writeResult(0),
// m_pauseResult(0),
// m_entropyMode(0),
// m_destroyFailures(0),
// m_active(false),
// m_syncConnect(false),
// m_createFail(false),
// m_fault(0),
// m_closeOnPause(false),
// m_dataOnResume(0),
// m_written(0),
// m_pauses(0),
// m_stops(0),
// m_destroys(0)
// {
//
// }
//
// NetHostFunctions NetHostSocketTestDriver::functions()
// {
//     NetHostFunctions functions = {};
//     functions.structSize = sizeof(functions);
//     functions.version = 1;
//     functions.context = this;
//     functions.create = create;
//     functions.listen = listen;
//     functions.connect = connect;
//     functions.poll = poll;
//     functions.write = write;
//     functions.pauseRead = pauseRead;
//     functions.close = close;
//     functions.shutdown = shutdown;
//     functions.destroy = destroy;
//     functions.localPort = localPort;
//     functions.peerCount = peerCount;
//     functions.entropy = entropy;
//     return functions;
// }
//
// int32_t NetHostSocketTestDriver::create(void* providerContext, void* observerContext, NetHostConnected connected,
//     NetHostData data, NetHostWritable writable, NetHostClosed closed, void** instance)
// {
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(providerContext);
//     *instance = nullptr;
//     if ((driver->m_fault & 1) != 0) { throw 1; }
//     if (driver->m_createFail)
//     {
//         return -1;
//     }
//     driver->m_observer = observerContext;
//     driver->m_connected = connected;
//     driver->m_data = data;
//     driver->m_writable = writable;
//     driver->m_closed = closed;
//     *instance = driver;
//     return 0;
// }
//
// int32_t NetHostSocketTestDriver::listen(void* instance, const char* address, uint32_t port, uint32_t maxPeers)
// {
//     (void)address;
//     (void)port;
//     (void)maxPeers;
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//     if ((driver->m_fault & 2) != 0) { throw 1; }
//     driver->m_active = true;
//     return 0;
// }
//
// int32_t NetHostSocketTestDriver::connect(void* instance, const char* address, uint32_t port, uint64_t* peer)
// {
//     (void)address;
//     (void)port;
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//     if ((driver->m_fault & 4) != 0) { throw 1; }
//     *peer = 1;
//     driver->m_active = true;
//     if (driver->m_syncConnect)
//     {
//         driver->m_connected(driver->m_observer, 1);
//     }
//     return 0;
// }
//
// int32_t NetHostSocketTestDriver::poll(void* instance)
// {
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//     if ((driver->m_fault & 8) != 0) { throw 1; }
//     return driver->m_active ? 1 : 0;
// }
//
// int32_t NetHostSocketTestDriver::write(void* instance, uint64_t peer, const uint8_t* data, uint32_t size)
// {
//     (void)peer;
//     (void)data;
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//     if ((driver->m_fault & 16) != 0) { throw 1; }
//     if (driver->m_writeResult == 0)
//     {
//         driver->m_written += size;
//     }
//     return driver->m_writeResult;
// }
//
// int32_t NetHostSocketTestDriver::pauseRead(void* instance, uint64_t peer, int32_t paused)
// {
//     (void)peer;
//     (void)paused;
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//     ++driver->m_pauses;
//     if ((driver->m_fault & 32) != 0) { throw 1; }
//     if (paused != 0 && driver->m_closeOnPause)
//     {
//         driver->m_closeOnPause = false;
//         driver->m_closed(driver->m_observer, peer, 0);
//     }
//     if (paused == 0 && driver->m_dataOnResume != 0)
//     {
//         --driver->m_dataOnResume;
//         uint8_t bytes[64];
//         std::memset(bytes, static_cast<uint8_t>(driver->m_dataOnResume), sizeof(bytes));
//         driver->m_data(driver->m_observer, peer, bytes, sizeof(bytes));
//         std::memset(bytes, 0, sizeof(bytes));
//     }
//     return driver->m_pauseResult;
// }
//
// int32_t NetHostSocketTestDriver::close(void* instance, uint64_t peer, int32_t reason)
// {
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//     if ((driver->m_fault & 64) != 0) { throw 1; }
//     driver->m_closed(driver->m_observer, peer, reason);
//     return 0;
// }
//
// int32_t NetHostSocketTestDriver::shutdown(void* instance)
// {
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//     driver->m_active = false;
//     ++driver->m_stops;
//     if ((driver->m_fault & 128) != 0) { throw 1; }
//     return 0;
// }
//
// int32_t NetHostSocketTestDriver::destroy(void* instance)
// {
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//     ++driver->m_destroys;
//     if ((driver->m_fault & 256) != 0) { throw 1; }
//     if (driver->m_destroyFailures != 0)
//     {
//         --driver->m_destroyFailures;
//         return -1;
//     }
//     return 0;
// }
//
// uint32_t NetHostSocketTestDriver::localPort(void* instance)
// {
//     (void)instance;
//     if ((static_cast<NetHostSocketTestDriver*>(instance)->m_fault & 512) != 0) { throw 1; }
//     return 14327;
// }
//
// uint32_t NetHostSocketTestDriver::peerCount(void* instance)
// {
//     return static_cast<NetHostSocketTestDriver*>(instance)->m_active ? 1 : 0;
// }
//
// int32_t NetHostSocketTestDriver::entropy(void* context, uint8_t* output, uint32_t size)
// {
//     NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(context);
//     if (driver->m_entropyMode != 0)
//     {
//         if (size != 0)
//         {
//             output[0] = 0x7c;
//         }
//         if (driver->m_entropyMode == 2)
//         {
//             throw 1;
//         }
//         return -1;
//     }
//     std::memset(output, 0x6d, size);
//     return 0;
// }
//
// #include "NetPeer/NetRelayTransportTest.h"
// #include <algorithm>
// #if defined(_WIN32) && defined(_DEBUG)
// #include <crtdbg.h>
// #endif
//
// NetRelayTransportTestDriver::NetRelayTransportTestDriver() :
// m_capture(true)
// {
//
// }
//
// NetHostFunctions NetRelayTransportTestDriver::relayFunctions()
// {
//     NetHostFunctions result = functions();
//     result.write = capture;
//     return result;
// }
//
// int32_t NetRelayTransportTestDriver::capture(void* instance, uint64_t peer, const uint8_t* data, uint32_t size)
// {
//     const int32_t result = NetHostSocketTestDriver::write(instance, peer, data, size);
//     NetRelayTransportTestDriver* driver = static_cast<NetRelayTransportTestDriver*>(instance);
//     if (result == 0 && driver->m_capture)
//     {
//         driver->m_wire.insert(driver->m_wire.end(), data, data + size);
//     }
//     return result;
// }
//
// NetRelayTransportTestListener::NetRelayTransportTestListener() :
// m_relay(nullptr),
// m_mode(0),
// m_reject(0),
// m_connected(0),
// m_records(0),
// m_attempts(0),
// m_closed(0),
// m_reason(0),
// m_valid(true)
// {
//
// }
//
// void NetRelayTransportTestListener::onRelayConnected(uint64_t peer)
// {
//     ++m_connected;
//     m_relay->poll();
//     if (m_mode == 1)
//     {
//         m_relay->shutdown();
//     }
//     else if (m_mode == 2)
//     {
//         m_relay->close(peer, NI_CANCELLED);
//         m_relay->close(peer, NI_TIMEOUT);
//     }
//     else if (m_mode == 3)
//     {
//         throw 1;
//     }
// }
//
// bool NetRelayTransportTestListener::onRelayRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
// {
//     ++m_attempts;
//     if (m_reject != 0)
//     {
//         if (m_rejected.empty())
//         {
//             m_rejected.assign(record.m_data, record.m_data + record.m_size);
//         }
//         m_valid = m_valid && m_rejected.size() == record.m_size &&
//             std::memcmp(m_rejected.data(), record.m_data, record.m_size) == 0;
//         --m_reject;
//         return false;
//     }
//     ++m_records;
//     if (record.m_size != 0)
//     {
//         m_received.insert(m_received.end(), record.m_data, record.m_data + record.m_size);
//     }
//     m_relay->poll();
//     if (m_mode == 4)
//     {
//         m_relay->shutdown();
//     }
//     else if (m_mode == 5)
//     {
//         m_relay->close(peer, NI_CANCELLED);
//         m_relay->close(peer, NI_TIMEOUT);
//     }
//     else if (m_mode == 6)
//     {
//         throw 1;
//     }
//     return true;
// }
//
// void NetRelayTransportTestListener::onRelayClosed(uint64_t peer, int32_t reason)
// {
//     ++m_closed;
//     m_reason = reason;
//     m_valid = m_valid && m_relay->outputState(peer) == NI_INVALID_STATE;
//     m_relay->poll();
//     m_relay->close(peer);
//     if (m_mode == 7)
//     {
//         m_relay->shutdown();
//         throw 1;
//     }
// }
//
// #if defined(_WIN32) && defined(_DEBUG)
// static int32_t s_relayFailAfter = -1;
// static uint32_t s_relayAllocations = 0;
// /** 只在测试调用窗口计数/拒绝普通堆分配，不影响CRT自身记录
// @param [in] operation CRT分配操作
// @param [in] data CRT传入的旧指针
// @param [in] size 申请字节
// @param [in] block CRT块类型
// @param [in] request 申请序号
// @param [in] file 调试来源
// @param [in] line 调试行号
// @return 1允许或0拒绝
// */
// static int __cdecl RelayAllocationHook(int operation, void* data, size_t size, int block, long request, const unsigned char* file, int line)
// {
//     (void)data;
//     (void)size;
//     (void)request;
//     (void)file;
//     (void)line;
//     if (block == _NORMAL_BLOCK && (operation == _HOOK_ALLOC || operation == _HOOK_REALLOC))
//     {
//         ++s_relayAllocations;
//         if (s_relayFailAfter == 0)
//         {
//             return 0;
//         }
//         if (s_relayFailAfter > 0)
//         {
//             --s_relayFailAfter;
//         }
//     }
//     return 1;
// }
// #endif
//
// #define RELAY_CHECK(condition) do { ++checks; if (!(condition)) { std::printf("Relay failed line=%d checks=%u\n", __LINE__, checks); return 1; } } while (0)
//
// int main()
// {
//     uint32_t checks = 0;
//     NetRelayTransportTestDriver driver;
//     driver.m_syncConnect = true;
//     NetHostFunctions functions = driver.relayFunctions();
//     RELAY_CHECK(NetPeer::configureHost(functions) == NI_OK);
//     uint8_t payload[16384];
//     for (uint32_t index = 0; index < sizeof(payload); ++index)
//     {
//         payload[index] = static_cast<uint8_t>(index * 7 + 19);
//     }
//     NetInterfaceTunnelRecord data = {};
//     data.m_kind = NI_TUNNEL_BYTES;
//     data.m_product = 7;
//     data.m_channel = 1;
//     data.m_data = payload;
//     data.m_size = 1;
//     NetInterfaceTunnelRecord control = data;
//     control.m_kind = NI_TUNNEL_CONTROL;
//     control.m_product = 0;
//     control.m_channel = 0;
//     uint8_t encoded[16408];
//     uint32_t required = 0;
//     RELAY_CHECK(NetInterfaceTunnel::encode(data, encoded, sizeof(encoded), required) == NI_OK);
// #if defined(_WIN32) && defined(_DEBUG)
//     _CrtMemState memoryBefore;
//     _CrtMemCheckpoint(&memoryBefore);
// #endif
//
//     // 独立数量额度、两类共用FIFO及回绕；背压期间拒绝不得改变已接纳字节。
//     {
//         NetRelayTransportTestListener listener;
//         NetRelayTransport relay;
//         listener.m_relay = &relay;
//         uint64_t peer = 0;
//         RELAY_CHECK(relay.connect("127.0.0.1", 1, &listener, peer) == NI_OK);
//         RELAY_CHECK(relay.send(peer, data) == NI_INVALID_STATE);
//         relay.poll();
//         RELAY_CHECK(listener.m_connected == 1);
//         for (uint32_t round = 0; round < 11; ++round)
//         {
//             driver.m_wire.clear();
//             std::vector<uint8_t> expected;
//             driver.m_writeResult = 1;
//             for (uint32_t index = 0; index < 64; ++index)
//             {
//                 payload[0] = static_cast<uint8_t>(index + round);
//                 NetInterfaceTunnelRecord record = (index % 2 == 0) ? data : control;
//                 RELAY_CHECK(NetInterfaceTunnel::encode(record, encoded, sizeof(encoded), required) == NI_OK);
//                 expected.insert(expected.end(), encoded, encoded + required);
//                 RELAY_CHECK(relay.send(peer, record) == NI_OK);
//             }
//             RELAY_CHECK(relay.send(peer, data) == NI_LIMIT_EXCEEDED);
//             RELAY_CHECK(relay.send(peer, control) == NI_LIMIT_EXCEEDED);
//             relay.poll();
//             RELAY_CHECK(driver.m_wire.empty() && relay.outputState(peer) == NI_READY);
//             payload[0] ^= 0xff;
//             driver.m_writeResult = NI_OK;
//             relay.poll();
//             RELAY_CHECK(driver.m_wire == expected && relay.outputState(peer) == NI_OK);
//         }
//         // 字节预算恰好命中：4条最大业务和2条最大控制；拒绝后队首不变。
//         driver.m_wire.clear();
//         data.m_size = sizeof(payload);
//         control.m_size = sizeof(payload);
//         for (uint32_t index = 0; index < 4; ++index)
//         {
//             RELAY_CHECK(relay.send(peer, data) == NI_OK);
//         }
//         RELAY_CHECK(relay.send(peer, data) == NI_LIMIT_EXCEEDED);
//         for (uint32_t index = 0; index < 2; ++index)
//         {
//             RELAY_CHECK(relay.send(peer, control) == NI_OK);
//         }
//         RELAY_CHECK(relay.send(peer, control) == NI_LIMIT_EXCEEDED);
//         relay.poll();
//         RELAY_CHECK(driver.m_wire.size() == 6 * 16408 && relay.outputState(peer) == NI_OK);
//         NetInterfaceTunnelRecord bad = data;
//         bad.m_size = 16385;
//         RELAY_CHECK(relay.send(peer, bad) < 0 && relay.outputState(peer) == NI_OK);
//         bad = data;
//         bad.m_data = nullptr;
//         RELAY_CHECK(relay.send(peer, bad) == NI_INVALID_ARGUMENT);
//         relay.shutdown();
//         relay.shutdown();
//         RELAY_CHECK(listener.m_closed == 1 && listener.m_valid && !relay.poll());
//         RELAY_CHECK(relay.connect("127.0.0.1", 1, &listener, peer) < 0 && peer == 0);
//     }
//     // 每个截断位置都通过真实Host事件入口；EOF前的完整帧仍交付一次。
//     data.m_size = 33;
//     RELAY_CHECK(NetInterfaceTunnel::encode(data, encoded, sizeof(encoded), required) == NI_OK);
//     for (uint32_t length = 0; length <= required; ++length)
//     {
//         NetRelayTransportTestListener listener;
//         NetRelayTransport relay;
//         listener.m_relay = &relay;
//         uint64_t peer = 0;
//         RELAY_CHECK(relay.connect("127.0.0.1", 1, &listener, peer) == NI_OK);
//         relay.poll();
//         for (uint32_t index = 0; index < length; ++index)
//         {
//             driver.m_data(driver.m_observer, peer, encoded + index, 1);
//             relay.poll();
//         }
//         driver.m_closed(driver.m_observer, peer, 0);
//         for (uint32_t spin = 0; spin < 8; ++spin)
//         {
//             relay.poll();
//         }
//         RELAY_CHECK(listener.m_closed == 1 && listener.m_valid);
//         RELAY_CHECK(listener.m_reason == ((length == 0 || length == required) ? NI_OK : NI_TRUNCATED));
//         RELAY_CHECK(listener.m_records == (length == required ? 1U : 0U));
//         RELAY_CHECK(length != required || listener.m_received == std::vector<uint8_t>(payload, payload + data.m_size));
//     }
//     // 上层连续拒绝且同时EOF：同一记录不变，不提前关闭，不重复接纳。
//     {
//         NetRelayTransportTestListener listener;
//         NetRelayTransport relay;
//         listener.m_relay = &relay;
//         listener.m_reject = 3;
//         uint64_t peer = 0;
//         RELAY_CHECK(relay.connect("127.0.0.1", 1, &listener, peer) == NI_OK);
//         relay.poll();
//         driver.m_data(driver.m_observer, peer, encoded, required);
//         driver.m_closed(driver.m_observer, peer, 0);
//         for (uint32_t spin = 0; spin < 3; ++spin)
//         {
//             relay.poll();
//             RELAY_CHECK(listener.m_closed == 0 && listener.m_records == 0);
//         }
//         for (uint32_t spin = 0; spin < 8; ++spin)
//         {
//             relay.poll();
//         }
//         RELAY_CHECK(listener.m_valid && listener.m_records == 1 && listener.m_attempts == 4 && listener.m_closed == 1);
//     }
//     // 每个回调阶段的重入/异常和首个关闭原因，不可让异常离开poll。
//     for (uint32_t mode = 1; mode <= 7; ++mode)
//     {
//         NetRelayTransportTestListener listener;
//         NetRelayTransport relay;
//         listener.m_relay = &relay;
//         listener.m_mode = mode;
//         uint64_t peer = 0;
//         RELAY_CHECK(relay.connect("127.0.0.1", 1, &listener, peer) == NI_OK);
//         relay.poll();
//         if (mode >= 4 && mode <= 6)
//         {
//             driver.m_data(driver.m_observer, peer, encoded, required);
//         }
//         if (mode == 7)
//         {
//             relay.close(peer, NI_TIMEOUT);
//         }
//         for (uint32_t spin = 0; spin < 8; ++spin)
//         {
//             relay.poll();
//         }
//         RELAY_CHECK(listener.m_valid && listener.m_connected == 1 && listener.m_closed == 1);
//         const int32_t expected = mode == 2 || mode == 5 ? NI_CANCELLED : mode == 3 || mode == 6 ? NI_NO_MEMORY : mode == 7 ? NI_TIMEOUT : NI_OK;
//         RELAY_CHECK(listener.m_reason == expected);
//     }
//     // 宿主写失败、异常和暂停失败均收敛到一次终态，不永久占用队列。
//     for (uint32_t mode = 0; mode < 3; ++mode)
//     {
//         NetRelayTransportTestListener listener;
//         NetRelayTransport relay;
//         listener.m_relay = &relay;
//         uint64_t peer = 0;
//         RELAY_CHECK(relay.connect("127.0.0.1", 1, &listener, peer) == NI_OK);
//         relay.poll();
//         RELAY_CHECK(relay.send(peer, data) == NI_OK);
//         if (mode == 0)
//         {
//             driver.m_writeResult = -1234;
//         }
//         else if (mode == 1)
//         {
//             driver.m_fault = 16;
//         }
//         else
//         {
//             driver.m_pauseResult = -1234;
//             driver.m_data(driver.m_observer, peer, encoded, required);
//         }
//         for (uint32_t spin = 0; spin < 8; ++spin)
//         {
//             relay.poll();
//         }
//         RELAY_CHECK(listener.m_closed == 1 && listener.m_reason < 0 && listener.m_valid);
//         driver.m_writeResult = 0;
//         driver.m_pauseResult = 0;
//         driver.m_fault = 0;
//     }
//     // 底层监听故障后必须报告收敛，不能持续让上层空转。
//     {
//         NetRelayTransportTestListener listener;
//         NetRelayTransport relay;
//         listener.m_relay = &relay;
//         RELAY_CHECK(relay.listen("127.0.0.1", 0, &listener, 2) == NI_OK);
//         RELAY_CHECK(relay.poll());
//         driver.m_fault = 8;
//         const bool active = relay.poll();
//         driver.m_fault = 0;
//         RELAY_CHECK(!active && !relay.poll());
//         RELAY_CHECK(listener.m_closed == 0);
//     }
//     // 两条入站连接分别拥有队列；关闭其中一条不丢另一条已接纳数据。
//     {
//         NetRelayTransportTestListener listener;
//         NetRelayTransport relay;
//         listener.m_relay = &relay;
//         RELAY_CHECK(relay.listen("127.0.0.1", 0, &listener, 2) == NI_OK);
//         driver.m_connected(driver.m_observer, 1);
//         driver.m_connected(driver.m_observer, 2);
//         relay.poll();
//         RELAY_CHECK(listener.m_connected == 2);
//         driver.m_wire.clear();
//         RELAY_CHECK(relay.send(1, data) == NI_OK && relay.send(2, data) == NI_OK);
//         relay.close(1, NI_CANCELLED);
//         relay.poll();
//         RELAY_CHECK(listener.m_closed == 1 && listener.m_reason == NI_CANCELLED);
//         RELAY_CHECK(relay.outputState(2) == NI_OK && driver.m_wire.size() == data.m_size + 24);
//         relay.shutdown();
//         RELAY_CHECK(listener.m_closed == 2 && listener.m_valid);
//     }
// #if defined(_WIN32) && defined(_DEBUG)
//     // 新缓冲分配失败后不扣额度；恢复后发送同一记录应成功。
//     {
//         NetRelayTransportTestListener listener;
//         NetRelayTransport relay;
//         listener.m_relay = &relay;
//         uint64_t peer = 0;
//         RELAY_CHECK(relay.connect("127.0.0.1", 1, &listener, peer) == NI_OK);
//         relay.poll();
//         s_relayFailAfter = 0;
//         _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(RelayAllocationHook);
//         const int32_t result = relay.send(peer, data);
//         _CrtSetAllocHook(previous);
//         s_relayFailAfter = -1;
//         RELAY_CHECK(result == NI_NO_MEMORY && relay.outputState(peer) == NI_OK);
//         RELAY_CHECK(relay.send(peer, data) == NI_OK);
//         relay.poll();
//         RELAY_CHECK(relay.outputState(peer) == NI_OK);
//     }
// #endif
//     // 预热后10万次真实send/poll，独立检查交付字节和Debug正常堆分配数。
//     driver.m_capture = false;
//     {
//         NetRelayTransportTestListener listener;
//         NetRelayTransport relay;
//         listener.m_relay = &relay;
//         uint64_t peer = 0;
//         RELAY_CHECK(relay.connect("127.0.0.1", 1, &listener, peer) == NI_OK);
//         relay.poll();
//         data.m_size = 1024;
//         RELAY_CHECK(relay.send(peer, data) == NI_OK);
//         relay.poll();
//         const uint32_t before = driver.m_written;
// #if defined(_WIN32) && defined(_DEBUG)
//         s_relayAllocations = 0;
//         _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(RelayAllocationHook);
// #endif
//         bool valid = true;
//         for (uint32_t index = 0; index < 100000; ++index)
//         {
//             valid = valid && relay.send(peer, data) == NI_OK;
//             relay.poll();
//             valid = valid && relay.outputState(peer) == NI_OK;
//         }
// #if defined(_WIN32) && defined(_DEBUG)
//         _CrtSetAllocHook(previous);
//         RELAY_CHECK(s_relayAllocations == 0);
// #endif
//         RELAY_CHECK(valid && driver.m_written - before == 100000 * 1048);
//     }
//     std::vector<uint8_t>().swap(driver.m_wire);
// #if defined(_WIN32) && defined(_DEBUG)
//     _CrtMemState memoryAfter;
//     _CrtMemState memoryDifference;
//     _CrtMemCheckpoint(&memoryAfter);
//     _CrtMemDifference(&memoryDifference, &memoryBefore, &memoryAfter);
//     RELAY_CHECK(memoryDifference.lCounts[_NORMAL_BLOCK] == 0 && memoryDifference.lSizes[_NORMAL_BLOCK] == 0);
// #endif
//     std::printf("Relay transport checks=%u PASS\n", checks);
//     return 0;
// }
// NET_RELAY_UNIT_END