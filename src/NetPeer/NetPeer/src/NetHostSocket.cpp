#include "NetHostSocket.h"
#include "NetPeerMacro.h"
#include "NetHostSocketState.h"
#include <cstring>
#include <utility>
#include "NetInterface/NetInterfaceAPI.h"

NetHostSocketListener::~NetHostSocketListener()
{

}

NetHostSocket::NetHostSocket(const NetHostFunctions* functions) :
m_state(new NetHostSocketState(functions == nullptr ? NetHostSocketState::m_default :
    (NetHostSocketState::valid(*functions) ? *functions : NetHostFunctions())))
{

}

NetHostSocket::~NetHostSocket()
{
    const int32_t result = uninit();
    m_state->m_listener = nullptr;
    if (result == NI_OK)
    {
        delete m_state;
    }
    // 违约宿主拒绝释放时保留已停止/无监听者的有界状态，避免迟到回调使用已释放内存
}

int32_t NetHostSocket::configureDefault(const NetHostFunctions& functions)
{
    if (!NetHostSocketState::valid(functions))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (NetHostSocketState::m_configured)
    {
        return NI_INVALID_STATE;
    }
    NetHostSocketState::m_default = functions;
    NetHostSocketState::m_configured = true;
    return NI_OK;
}

bool NetHostSocket::defaultSupports(bool server)
{
    return NetHostSocketState::m_configured &&
        (server ? NetHostSocketState::m_default.listen != nullptr : NetHostSocketState::m_default.connect != nullptr);
}

int32_t NetHostSocket::listen(const char* address, uint32_t port, NetHostSocketListener* listener, uint32_t maxPeers)
{
    if (address == nullptr || listener == nullptr || port > 65535 || maxPeers == 0 || maxPeers > 256)
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t started = m_state->start(listener, maxPeers, true);
    if (started != NI_OK)
    {
        return started;
    }
    int32_t result = -1;
    try
    {
        result = m_state->m_functions.listen(m_state->m_instance, address, port, maxPeers);
    }
    catch (...)
    {
        m_state->fail(NI_INVALID_STATE);
    }
    if (result != 0 || m_state->m_fault != NI_OK)
    {
        m_state->m_listener = nullptr;
        m_state->stop();
        return NI_INVALID_STATE;
    }
    m_state->m_phase = NET_HOST_ACTIVE;
    return NI_OK;
}

int32_t NetHostSocket::connect(const char* address, uint32_t port, NetHostSocketListener* listener, uint64_t& peer)
{
    peer = 0;
    if (address == nullptr || listener == nullptr || port == 0 || port > 65535)
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t started = m_state->start(listener, 1, false);
    if (started != NI_OK)
    {
        return started;
    }
    int32_t result = -1;
    try
    {
        result = m_state->m_functions.connect(m_state->m_instance, address, port, &peer);
        if (result == 0 && peer != 0 && m_state->m_peers.find(peer) == m_state->m_peers.end())
        {
            if (!m_state->m_peers.empty() || peer <= m_state->m_lastPeer)
            {
                m_state->fail(NI_PROTOCOL_ERROR);
            }
            else
            {
                m_state->m_peers.insert(std::make_pair(peer, NetHostSocketPeer()));
                m_state->m_lastPeer = peer;
            }
        }
    }
    catch (...)
    {
        m_state->fail(NI_INVALID_STATE);
    }
    if (result != 0 || peer == 0 || m_state->m_fault != NI_OK)
    {
        peer = 0;
        m_state->m_listener = nullptr;
        m_state->stop();
        return NI_INVALID_STATE;
    }
    m_state->m_phase = NET_HOST_ACTIVE;
    return NI_OK;
}

bool NetHostSocket::poll()
{
    return m_state->poll();
}

int32_t NetHostSocket::write(uint64_t peer, const uint8_t* data, uint32_t size)
{
    if ((data == nullptr && size != 0) || size > 32768)
    {
        return NI_INVALID_ARGUMENT;
    }
    auto found = m_state->m_peers.find(peer);
    if (m_state->m_phase != NET_HOST_ACTIVE || found == m_state->m_peers.end() || !found->second.m_announced ||
        found->second.m_closing || found->second.m_closed)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        const int32_t result = m_state->m_functions.write(m_state->m_instance, peer, data, size);
        if (result == 0)
        {
            return NI_OK;
        }
        if (result == 1)
        {
            return NI_LIMIT_EXCEEDED;
        }
        if (result > 1)
        {
            m_state->fail(NI_PROTOCOL_ERROR);
        }
        return NI_INVALID_STATE;
    }
    catch (...)
    {
        m_state->fail(NI_INVALID_STATE);
        return NI_INVALID_STATE;
    }
}

int32_t NetHostSocket::pauseRead(uint64_t peer, bool paused)
{
    auto found = m_state->m_peers.find(peer);
    if (m_state->m_phase != NET_HOST_ACTIVE || found == m_state->m_peers.end() || !found->second.m_connected ||
        found->second.m_closing || (!paused && found->second.m_inputPending))
    {
        return NI_INVALID_STATE;
    }
    found->second.m_paused = paused;
    // 已观察EOF但终态尚在排队：仅控制最后输入的本地交付，不再操作底层socket。
    if (found->second.m_closed) { return NI_OK; }
    try
    {
        if (m_state->m_functions.pauseRead(m_state->m_instance, peer, paused ? 1 : 0) != 0)
        {
            m_state->fail(NI_INVALID_STATE);
            return NI_INVALID_STATE;
        }
        return NI_OK;
    }
    catch (...)
    {
        m_state->fail(NI_INVALID_STATE);
        return NI_INVALID_STATE;
    }
}

void NetHostSocket::close(uint64_t peer, int32_t reason)
{
    m_state->close(peer, reason);
}

void NetHostSocket::shutdown()
{
    if (m_state->m_phase != NET_HOST_STOPPED)
    {
        m_state->m_phase = NET_HOST_STOPPING;
    }
    if (!m_state->m_polling && !m_state->m_callback)
    {
        m_state->stop();
    }
}

int32_t NetHostSocket::uninit()
{
    if (m_state->m_polling || m_state->m_callback)
    {
        return NI_INVALID_STATE;
    }
    shutdown();
    if (m_state->m_instance != nullptr)
    {
        try
        {
            if (m_state->m_functions.destroy(m_state->m_instance) != 0)
            {
                m_state->fail(NI_INVALID_STATE);
                return NI_INVALID_STATE;
            }
        }
        catch (...)
        {
            m_state->fail(NI_INVALID_STATE);
            return NI_INVALID_STATE;
        }
        m_state->m_instance = nullptr;
    }
    return NI_OK;
}

int32_t NetHostSocket::status() const
{
    return m_state->m_fault;
}

uint32_t NetHostSocket::localPort() const
{
    if (m_state->m_instance == nullptr || m_state->m_phase != NET_HOST_ACTIVE)
    {
        return 0;
    }
    try
    {
        return m_state->m_functions.localPort(m_state->m_instance);
    }
    catch (...)
    {
        return 0;
    }
}

uint32_t NetHostSocket::peerCount() const
{
    return static_cast<uint32_t>(m_state->m_peers.size());
}

bool NetHostSocket::isBackpressure(int32_t result)
{
    return result == NI_LIMIT_EXCEEDED;
}

int32_t NetHostSocket::randomBytes(uint8_t* output, uint32_t size)
{
    if ((output == nullptr && size != 0) || size > 8192)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (size == 0)
    {
        return NI_OK;
    }
    try
    {
        if (NetHostSocketState::m_configured && NetHostSocketState::m_default.entropy != nullptr &&
            NetHostSocketState::m_default.entropy(NetHostSocketState::m_default.context, output, size) == 0)
        {
            return NI_OK;
        }
    }
    catch (...)
    {
        // 宿主异常按熵失败处理，不把未完成的输出用于TLS或会话代次
    }
    volatile uint8_t* target = output;
    for (uint32_t index = 0; index < size; ++index)
    {
        target[index] = 0;
    }
    return NI_INVALID_STATE;
}

// NET_HOST_SOCKET_UNIT_TEST_BEGIN
//
//#include "NetPeer/NetHostSocketTest.h"
//#include "NetPeer.h"
//#include <cstring>
//#include <chrono>
//#include <cstdio>
//
//NetHostSocketTestDriver::NetHostSocketTestDriver() :
//m_observer(nullptr),
//m_connected(nullptr),
//m_data(nullptr),
//m_writable(nullptr),
//m_closed(nullptr),
//m_writeResult(0),
//m_pauseResult(0),
//m_entropyMode(0),
//m_destroyFailures(0),
//m_active(false),
//m_syncConnect(false),
//m_createFail(false),
//m_fault(0),
//m_closeOnPause(false),
//m_dataOnResume(0),
//m_written(0),
//m_pauses(0),
//m_stops(0),
//m_destroys(0)
//{
//
//}
//
//NetHostFunctions NetHostSocketTestDriver::functions()
//{
//    NetHostFunctions functions = {};
//    functions.structSize = sizeof(functions);
//    functions.version = 1;
//    functions.context = this;
//    functions.create = create;
//    functions.listen = listen;
//    functions.connect = connect;
//    functions.poll = poll;
//    functions.write = write;
//    functions.pauseRead = pauseRead;
//    functions.close = close;
//    functions.shutdown = shutdown;
//    functions.destroy = destroy;
//    functions.localPort = localPort;
//    functions.peerCount = peerCount;
//    functions.entropy = entropy;
//    return functions;
//}
//
//int32_t NetHostSocketTestDriver::create(void* providerContext, void* observerContext, NetHostConnected connected,
//    NetHostData data, NetHostWritable writable, NetHostClosed closed, void** instance)
//{
//    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(providerContext);
//    *instance = nullptr;
//    if ((driver->m_fault & 1) != 0) { throw 1; }
//    if (driver->m_createFail)
//    {
//        return -1;
//    }
//    driver->m_observer = observerContext;
//    driver->m_connected = connected;
//    driver->m_data = data;
//    driver->m_writable = writable;
//    driver->m_closed = closed;
//    *instance = driver;
//    return 0;
//}
//
//int32_t NetHostSocketTestDriver::listen(void* instance, const char* address, uint32_t port, uint32_t maxPeers)
//{
//    (void)address;
//    (void)port;
//    (void)maxPeers;
//    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//    if ((driver->m_fault & 2) != 0) { throw 1; }
//    driver->m_active = true;
//    return 0;
//}
//
//int32_t NetHostSocketTestDriver::connect(void* instance, const char* address, uint32_t port, uint64_t* peer)
//{
//    (void)address;
//    (void)port;
//    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//    if ((driver->m_fault & 4) != 0) { throw 1; }
//    *peer = 1;
//    driver->m_active = true;
//    if (driver->m_syncConnect)
//    {
//        driver->m_connected(driver->m_observer, 1);
//    }
//    return 0;
//}
//
//int32_t NetHostSocketTestDriver::poll(void* instance)
//{
//    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//    if ((driver->m_fault & 8) != 0) { throw 1; }
//    return driver->m_active ? 1 : 0;
//}
//
//int32_t NetHostSocketTestDriver::write(void* instance, uint64_t peer, const uint8_t* data, uint32_t size)
//{
//    (void)peer;
//    (void)data;
//    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//    if ((driver->m_fault & 16) != 0) { throw 1; }
//    if (driver->m_writeResult == 0)
//    {
//        driver->m_written += size;
//    }
//    return driver->m_writeResult;
//}
//
//int32_t NetHostSocketTestDriver::pauseRead(void* instance, uint64_t peer, int32_t paused)
//{
//    (void)peer;
//    (void)paused;
//    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//    ++driver->m_pauses;
//    if ((driver->m_fault & 32) != 0) { throw 1; }
//    if (paused != 0 && driver->m_closeOnPause)
//    {
//        driver->m_closeOnPause = false;
//        driver->m_closed(driver->m_observer, peer, 0);
//    }
//    if (paused == 0 && driver->m_dataOnResume != 0)
//    {
//        --driver->m_dataOnResume;
//        uint8_t bytes[64];
//        std::memset(bytes, static_cast<uint8_t>(driver->m_dataOnResume), sizeof(bytes));
//        driver->m_data(driver->m_observer, peer, bytes, sizeof(bytes));
//        std::memset(bytes, 0, sizeof(bytes));
//    }
//    return driver->m_pauseResult;
//}
//
//int32_t NetHostSocketTestDriver::close(void* instance, uint64_t peer, int32_t reason)
//{
//    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//    if ((driver->m_fault & 64) != 0) { throw 1; }
//    driver->m_closed(driver->m_observer, peer, reason);
//    return 0;
//}
//
//int32_t NetHostSocketTestDriver::shutdown(void* instance)
//{
//    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//    driver->m_active = false;
//    ++driver->m_stops;
//    if ((driver->m_fault & 128) != 0) { throw 1; }
//    return 0;
//}
//
//int32_t NetHostSocketTestDriver::destroy(void* instance)
//{
//    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
//    ++driver->m_destroys;
//    if ((driver->m_fault & 256) != 0) { throw 1; }
//    if (driver->m_destroyFailures != 0)
//    {
//        --driver->m_destroyFailures;
//        return -1;
//    }
//    return 0;
//}
//
//uint32_t NetHostSocketTestDriver::localPort(void* instance)
//{
//    (void)instance;
//    if ((static_cast<NetHostSocketTestDriver*>(instance)->m_fault & 512) != 0) { throw 1; }
//    return 14327;
//}
//
//uint32_t NetHostSocketTestDriver::peerCount(void* instance)
//{
//    return static_cast<NetHostSocketTestDriver*>(instance)->m_active ? 1 : 0;
//}
//
//int32_t NetHostSocketTestDriver::entropy(void* context, uint8_t* output, uint32_t size)
//{
//    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(context);
//    if (driver->m_entropyMode != 0)
//    {
//        if (size != 0)
//        {
//            output[0] = 0x7c;
//        }
//        if (driver->m_entropyMode == 2)
//        {
//            throw 1;
//        }
//        return -1;
//    }
//    std::memset(output, 0x6d, size);
//    return 0;
//}
//
//NetHostSocketTestListener::NetHostSocketTestListener() :
//m_socket(nullptr),
//m_connected(0),
//m_data(0),
//m_writable(0),
//m_closed(0),
//m_reason(0),
//m_detail(0),
//m_stopOnConnect(false),
//m_throwOnConnect(false),
//m_resumeOnData(false),
//m_throwOnData(false),
//m_trace(false),
//m_valid(true)
//{
//
//}
//
//void NetHostSocketTestListener::onConnected(uint64_t peer)
//{
//    ++m_connected;
//    if (m_trace) { m_events.push_back(1); }
//    m_valid = m_valid && peer != 0;
//    m_socket->poll();
//    m_valid = m_valid && m_socket->uninit() == NI_INVALID_STATE;
//    if (m_stopOnConnect)
//    {
//        m_socket->shutdown();
//    }
//    if (m_throwOnConnect)
//    {
//        throw 1;
//    }
//}
//
//void NetHostSocketTestListener::onData(uint64_t peer, const uint8_t* data, uint32_t size)
//{
//    ++m_data;
//    if (m_trace) { m_events.push_back(2); }
//    m_valid = m_valid && peer != 0 && data != nullptr && size != 0 && size <= 16384;
//    const size_t begin = m_bytes.size();
//    m_bytes.insert(m_bytes.end(), data, data + size);
//    if (m_resumeOnData)
//    {
//        m_valid = m_valid && m_socket->pauseRead(peer, false) == NI_OK;
//        m_valid = m_valid && std::memcmp(data, &m_bytes[begin], size) == 0;
//    }
//    if (m_throwOnData) { throw 1; }
//}
//
//void NetHostSocketTestListener::onWritable(uint64_t peer)
//{
//    m_valid = m_valid && peer != 0;
//    ++m_writable;
//    if (m_trace) { m_events.push_back(3); }
//}
//
//void NetHostSocketTestListener::onClosed(uint64_t peer, int32_t reason, int32_t detail)
//{
//    m_valid = m_valid && peer != 0;
//    ++m_closed;
//    if (m_trace) { m_events.push_back(4); }
//    m_reason = reason;
//    m_detail = detail;
//    m_socket->poll();
//}
//
//
//#if defined(_WIN32) && defined(_DEBUG)
//#include <crtdbg.h>
//static int32_t g_hostRejectAfter = -1;
//static uint32_t g_hostAllocations = 0;
//
///** 单元专用分配故障；非负值表示再允许指定次数后拒绝
//@param [in] operation CRT操作
//@param [in] data 未使用
//@param [in] size 未使用
//@param [in] block 内存类别
//@param [in] request 未使用
//@param [in] file 未使用
//@param [in] line 未使用
//@return 是否允许分配
//*/
//static int HostMemoryHook(int operation, void*, size_t, int block, long, const unsigned char*, int)
//{
//    if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK)
//    {
//        ++g_hostAllocations;
//        if (g_hostRejectAfter == 0) { return 0; }
//        if (g_hostRejectAfter > 0) { --g_hostRejectAfter; }
//    }
//    return 1;
//}
//#endif
//int main()
//{
//    uint32_t assertions = 0;
//#define SOCKET_CHECK(condition) { ++assertions; if (!(condition)) { std::printf("FAIL line %u: %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
//#if defined(_WIN32) && defined(_DEBUG)
//    _CrtMemState before;
//    _CrtMemCheckpoint(&before);
//#endif
//    {
//    NetHostSocketTestListener emptyListener;
//    NetHostSocket missing;
//    uint8_t random[34];
//    std::memset(random, 0xa5, sizeof(random));
//    SOCKET_CHECK(NetHostSocket::randomBytes(nullptr, 0) == NI_OK);
//    SOCKET_CHECK(NetHostSocket::randomBytes(nullptr, 1) == NI_INVALID_ARGUMENT);
//    SOCKET_CHECK(NetHostSocket::randomBytes(random + 1, 8193) == NI_INVALID_ARGUMENT && random[1] == 0xa5);
//    SOCKET_CHECK(NetHostSocket::randomBytes(random + 1, 32) == NI_INVALID_STATE);
//    SOCKET_CHECK(random[0] == 0xa5 && random[33] == 0xa5);
//    for (uint32_t index = 1; index < 33; ++index)
//    {
//        SOCKET_CHECK(random[index] == 0);
//    }
//    uint64_t missingPeer = 3;
//    SOCKET_CHECK(missing.connect("127.0.0.1", 1, &emptyListener, missingPeer) == NI_NOT_SUPPORTED && missingPeer == 0);
//    SOCKET_CHECK(!NetHostSocket::defaultSupports(false) && !NetHostSocket::defaultSupports(true));
//    NetHostSocketTestDriver provider;
//    NetHostFunctions table = provider.functions();
//    {
//        NetHostFunctions clientOnly = table;
//        clientOnly.listen = nullptr;
//        NetHostSocket client(&clientOnly);
//        NetHostSocketTestListener listener;
//        listener.m_socket = &client;
//        SOCKET_CHECK(client.listen("127.0.0.1", 0, &listener) == NI_NOT_SUPPORTED && !provider.m_active);
//        SOCKET_CHECK(client.connect("127.0.0.1", 1, &listener, missingPeer) == NI_OK && missingPeer == 1);
//        SOCKET_CHECK(client.uninit() == NI_OK);
//    }
//    {
//        NetHostFunctions serverOnly = table;
//        serverOnly.connect = nullptr;
//        NetHostSocket server(&serverOnly);
//        NetHostSocketTestListener listener;
//        listener.m_socket = &server;
//        SOCKET_CHECK(server.connect("127.0.0.1", 1, &listener, missingPeer) == NI_NOT_SUPPORTED && missingPeer == 0 && !provider.m_active);
//        SOCKET_CHECK(server.listen("127.0.0.1", 0, &listener) == NI_OK && server.poll());
//        provider.m_active = false; // 已接纳的监听自行消失，即使没有peer也必须进入故障终态。
//        SOCKET_CHECK(!server.poll() && server.status() == NI_PROTOCOL_ERROR && listener.m_closed == 0);
//        SOCKET_CHECK(server.uninit() == NI_OK);
//    }
//    NetHostFunctions invalid = table;
//    invalid.listen = nullptr;
//    invalid.connect = nullptr;
//    SOCKET_CHECK(NetHostSocket::configureDefault(invalid) == NI_INVALID_ARGUMENT);
//    invalid = table;
//    invalid.structSize = 4;
//    SOCKET_CHECK(NetHostSocket::configureDefault(invalid) == NI_INVALID_ARGUMENT);
//    invalid = table;
//    invalid.version = 2;
//    SOCKET_CHECK(NetHostSocket::configureDefault(invalid) == NI_INVALID_ARGUMENT);
//    invalid = table;
//    invalid.write = nullptr;
//    SOCKET_CHECK(NetHostSocket::configureDefault(invalid) == NI_INVALID_ARGUMENT);
//    SOCKET_CHECK(NetHostSocket::configureDefault(table) == NI_OK);
//    SOCKET_CHECK(NetHostSocket::defaultSupports(false) && NetHostSocket::defaultSupports(true));
//    SOCKET_CHECK(NetHostSocket::configureDefault(table) == NI_INVALID_STATE);
//    SOCKET_CHECK(NetHostSocket::randomBytes(random + 1, 32) == NI_OK);
//    for (uint32_t index = 1; index < 33; ++index)
//    {
//        SOCKET_CHECK(random[index] == 0x6d);
//    }
//    for (uint32_t mode = 1; mode <= 2; ++mode)
//    {
//        provider.m_entropyMode = mode;
//        SOCKET_CHECK(NetHostSocket::randomBytes(random + 1, 32) == NI_INVALID_STATE);
//        for (uint32_t index = 1; index < 33; ++index)
//        {
//            SOCKET_CHECK(random[index] == 0);
//        }
//        SOCKET_CHECK(random[0] == 0xa5 && random[33] == 0xa5);
//    }
//    provider.m_entropyMode = 0;
//    SOCKET_CHECK(NetHostSocket::isBackpressure(NI_LIMIT_EXCEEDED));
//    SOCKET_CHECK(!NetHostSocket::isBackpressure(NI_INVALID_ARGUMENT));
//    for (uint32_t mode = 0; mode < 10; ++mode)
//    {
//        NetHostSocketTestDriver driver;
//        NetHostFunctions functions = driver.functions();
//        NetHostSocketTestListener listener;
//        driver.m_syncConnect = mode == 0;
//        {
//            NetHostSocket socket(&functions);
//            listener.m_socket = &socket;
//            uint64_t peer = 91;
//            SOCKET_CHECK(socket.connect(nullptr, 1, &listener, peer) == NI_INVALID_ARGUMENT && peer == 0);
//            SOCKET_CHECK(socket.connect("127.0.0.1", 0, &listener, peer) == NI_INVALID_ARGUMENT && peer == 0);
//            SOCKET_CHECK(socket.connect("127.0.0.1", 14327, &listener, peer) == NI_OK && peer == 1);
//            SOCKET_CHECK(socket.peerCount() == 1 && listener.m_connected == 0);
//            const uint8_t value = 0x35;
//            SOCKET_CHECK(socket.write(peer, &value, 1) == NI_INVALID_STATE);
//            if (!driver.m_syncConnect)
//            {
//                driver.m_connected(driver.m_observer, peer);
//            }
//            SOCKET_CHECK(listener.m_connected == 0 && driver.m_pauses == 1);
//            listener.m_stopOnConnect = mode == 1;
//            listener.m_throwOnConnect = mode == 2;
//            socket.poll();
//            SOCKET_CHECK(listener.m_connected == 1);
//            if (mode <= 2 && mode != 0)
//            {
//                socket.poll();
//                SOCKET_CHECK(listener.m_closed == 1 && socket.peerCount() == 0);
//            }
//            else
//            {
//                SOCKET_CHECK(socket.pauseRead(peer, false) == NI_OK);
//                if (mode == 0)
//                {
//                    driver.m_writeResult = 1;
//                    SOCKET_CHECK(socket.write(peer, &value, 1) == NI_LIMIT_EXCEEDED && driver.m_written == 0);
//                    driver.m_writeResult = 0;
//                    SOCKET_CHECK(socket.write(peer, &value, 1) == NI_OK && driver.m_written == 1);
//                    SOCKET_CHECK(socket.write(peer, nullptr, 1) == NI_INVALID_ARGUMENT && driver.m_written == 1);
//                    SOCKET_CHECK(socket.write(peer, &value, 32769) == NI_INVALID_ARGUMENT && driver.m_written == 1);
//                    for (uint32_t count = 0; count < 10000; ++count)
//                    {
//                        driver.m_writable(driver.m_observer, peer);
//                    }
//                    SOCKET_CHECK(listener.m_writable == 0);
//                    socket.poll();
//                    SOCKET_CHECK(listener.m_writable == 1);
//                    std::vector<uint8_t> bytes(16384, 0xc7);
//                    driver.m_data(driver.m_observer, peer, bytes.data(), static_cast<uint32_t>(bytes.size()));
//                    bytes.assign(16384, 0);
//                    SOCKET_CHECK(listener.m_data == 0);
//                    SOCKET_CHECK(socket.pauseRead(peer, false) == NI_INVALID_STATE);
//                    socket.poll();
//                    SOCKET_CHECK(listener.m_data == 1 && listener.m_bytes.size() == 16384);
//                    for (size_t index = 0; index < listener.m_bytes.size(); ++index)
//                    {
//                        SOCKET_CHECK(listener.m_bytes[index] == 0xc7);
//                    }
//                    SOCKET_CHECK(socket.pauseRead(peer, false) == NI_OK);
//                    socket.close(peer, 27);
//                    socket.close(peer, 28);
//                    SOCKET_CHECK(listener.m_closed == 0);
//                    socket.poll();
//                    SOCKET_CHECK(listener.m_closed == 1 && listener.m_reason == 27);
//                    driver.m_closed(driver.m_observer, peer, 29);
//                    driver.m_data(driver.m_observer, peer, &value, 1);
//                    driver.m_writable(driver.m_observer, peer);
//                    socket.poll();
//                    SOCKET_CHECK(listener.m_closed == 1 && listener.m_data == 1);
//                }
//                if (mode == 3)
//                {
//                    driver.m_data(driver.m_observer, peer, nullptr, 1);
//                }
//                if (mode == 4)
//                {
//                    driver.m_data(driver.m_observer, peer, &value, 16385);
//                }
//                if (mode == 5)
//                {
//                    driver.m_data(driver.m_observer, peer, &value, 1);
//                    driver.m_data(driver.m_observer, peer, &value, 1);
//                }
//                if (mode == 6)
//                {
//                    driver.m_data(driver.m_observer, peer + 100, &value, 1);
//                }
//                if (mode == 7)
//                {
//                    driver.m_pauseResult = -1;
//                    SOCKET_CHECK(socket.pauseRead(peer, true) == NI_INVALID_STATE);
//                }
//                if (mode == 8)
//                {
//                    driver.m_writeResult = 2;
//                    SOCKET_CHECK(socket.write(peer, &value, 1) == NI_INVALID_STATE);
//                }
//                if (mode == 9)
//                {
//                    driver.m_active = false;
//                }
//                socket.poll();
//                SOCKET_CHECK(listener.m_closed == 1 && socket.peerCount() == 0);
//                if (mode >= 3)
//                {
//                    SOCKET_CHECK(listener.m_data == 0);
//                    SOCKET_CHECK(listener.m_reason == (mode == 7 ? NI_INVALID_STATE : NI_PROTOCOL_ERROR));
//                }
//            }
//            socket.shutdown();
//            socket.shutdown();
//            socket.poll();
//            SOCKET_CHECK(listener.m_valid && listener.m_closed == 1);
//            SOCKET_CHECK(driver.m_destroys == 0);
//            SOCKET_CHECK(socket.uninit() == NI_OK && driver.m_destroys == 1);
//            SOCKET_CHECK(socket.uninit() == NI_OK && driver.m_destroys == 1);
//        }
//        SOCKET_CHECK(driver.m_destroys == 1 && driver.m_stops == 1);
//    }
//    {
//        NetHostSocketTestDriver driver;
//        NetHostFunctions functions = driver.functions();
//        NetHostSocketTestListener listener;
//        NetHostSocket socket(&functions);
//        listener.m_socket = &socket;
//        SOCKET_CHECK(socket.listen("127.0.0.1", 0, &listener, 257) == NI_INVALID_ARGUMENT);
//        SOCKET_CHECK(socket.listen("127.0.0.1", 0, &listener, 256) == NI_OK);
//        SOCKET_CHECK(socket.localPort() == 14327);
//        for (uint64_t peer = 1; peer <= 256; ++peer)
//        {
//            driver.m_connected(driver.m_observer, peer);
//        }
//        socket.poll();
//        SOCKET_CHECK(listener.m_connected == 256 && socket.peerCount() == 256);
//        std::vector<uint8_t> bytes(16384, 0x2d);
//        for (uint64_t peer = 1; peer <= 256; ++peer)
//        {
//            SOCKET_CHECK(socket.pauseRead(peer, false) == NI_OK);
//            driver.m_data(driver.m_observer, peer, bytes.data(), static_cast<uint32_t>(bytes.size()));
//        }
//        SOCKET_CHECK(listener.m_data == 0);
//        socket.poll();
//        SOCKET_CHECK(listener.m_data == 256 && listener.m_bytes.size() == 4194304);
//        driver.m_connected(driver.m_observer, 257);
//        socket.poll();
//        SOCKET_CHECK(listener.m_connected == 256 && listener.m_closed == 256 && socket.peerCount() == 0);
//    }
//    {
//        NetHostSocketTestDriver driver;
//        driver.m_createFail = true;
//        NetHostFunctions functions = driver.functions();
//        NetHostSocketTestListener listener;
//        NetHostSocket socket(&functions);
//        listener.m_socket = &socket;
//        uint64_t peer = 11;
//        SOCKET_CHECK(socket.connect("127.0.0.1", 14327, &listener, peer) == NI_INVALID_STATE && peer == 0);
//        SOCKET_CHECK(!socket.poll() && listener.m_closed == 0 && driver.m_destroys == 0);
//    }
//    // 原始宿主错误码与NI数值可相同，不能把原始-1解释成永久参数错误
//    for (int32_t raw = -13; raw <= 1; ++raw)
//    {
//        NetHostSocketTestDriver driver;
//        NetHostFunctions functions = driver.functions();
//        NetHostSocketTestListener listener;
//        NetHostSocket socket(&functions);
//        listener.m_socket = &socket;
//        uint64_t peer = 0;
//        SOCKET_CHECK(socket.connect("127.0.0.1", 14327, &listener, peer) == NI_OK);
//        driver.m_closed(driver.m_observer, peer, raw);
//        SOCKET_CHECK(listener.m_closed == 0);
//        socket.poll();
//        SOCKET_CHECK(listener.m_closed == 1 && listener.m_reason == (raw == 0 ? NI_OK : NI_INVALID_STATE));
//        SOCKET_CHECK(listener.m_detail == raw && socket.peerCount() == 0);
//        SOCKET_CHECK(socket.uninit() == NI_OK);
//    }
//    {
//        NetHostSocketTestDriver driver;
//        driver.m_destroyFailures = 1;
//        NetHostFunctions functions = driver.functions();
//        NetHostSocketTestListener listener;
//        NetHostSocket socket(&functions);
//        listener.m_socket = &socket;
//        uint64_t peer = 0;
//        SOCKET_CHECK(socket.connect("127.0.0.1", 14327, &listener, peer) == NI_OK);
//        SOCKET_CHECK(socket.uninit() == NI_INVALID_STATE && socket.status() == NI_INVALID_STATE);
//        SOCKET_CHECK(driver.m_destroys == 1 && driver.m_stops == 1 && listener.m_closed == 1);
//        driver.m_closed(driver.m_observer, peer, -1);
//        SOCKET_CHECK(listener.m_closed == 1 && !socket.poll());
//        SOCKET_CHECK(socket.uninit() == NI_OK && driver.m_destroys == 2 && driver.m_stops == 1);
//        SOCKET_CHECK(socket.uninit() == NI_OK && driver.m_destroys == 2 && listener.m_closed == 1);
//    }
//#ifdef NET_HOST_TEST_LIBUV
//    {
//        NetHostFunctions functions = {};
//        netHostSocketTestLibuvFunctions(&functions);
//        NetHostSocketTestListener serverListener;
//        NetHostSocketTestListener clientListener;
//        NetHostSocket server(&functions);
//        NetHostSocket client(&functions);
//        serverListener.m_socket = &server;
//        clientListener.m_socket = &client;
//        SOCKET_CHECK(server.listen("127.0.0.1", 0, &serverListener, 1) == NI_OK && server.localPort() != 0);
//        uint64_t peer = 0;
//        SOCKET_CHECK(client.connect("127.0.0.1", server.localPort(), &clientListener, peer) == NI_OK && peer != 0);
//        const std::chrono::steady_clock::time_point connectDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
//        while ((clientListener.m_connected == 0 || serverListener.m_connected == 0) && std::chrono::steady_clock::now() < connectDeadline)
//        {
//            client.poll();
//            server.poll();
//        }
//        SOCKET_CHECK(clientListener.m_connected == 1 && serverListener.m_connected == 1);
//        SOCKET_CHECK(server.pauseRead(1, false) == NI_OK);
//        std::vector<uint8_t> payload(32768, 0x96);
//        const int32_t firstWrite = client.write(peer, payload.data(), static_cast<uint32_t>(payload.size()));
//        if (firstWrite != NI_OK)
//        {
//            std::printf("firstWrite=%d peers=%u closed=%u reason=%d\n", firstWrite, client.peerCount(), clientListener.m_closed, clientListener.m_reason);
//        }
//        SOCKET_CHECK(firstWrite == NI_OK);
//        SOCKET_CHECK(client.write(peer, payload.data(), static_cast<uint32_t>(payload.size())) == NI_OK);
//        SOCKET_CHECK(client.write(peer, payload.data(), 1) == NI_LIMIT_EXCEEDED);
//        payload.assign(payload.size(), 0);
//        const std::chrono::steady_clock::time_point receiveDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
//        while (serverListener.m_bytes.size() < 65536 && std::chrono::steady_clock::now() < receiveDeadline)
//        {
//            client.poll();
//            server.poll();
//            server.pauseRead(1, false);
//        }
//        SOCKET_CHECK(serverListener.m_bytes.size() == 65536);
//        for (size_t index = 0; index < serverListener.m_bytes.size(); ++index)
//        {
//            SOCKET_CHECK(serverListener.m_bytes[index] == 0x96);
//        }
//        client.close(peer, 19);
//        const std::chrono::steady_clock::time_point closeDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
//        while ((clientListener.m_closed == 0 || serverListener.m_closed == 0) && std::chrono::steady_clock::now() < closeDeadline)
//        {
//            client.poll();
//            server.poll();
//        }
//        SOCKET_CHECK(clientListener.m_closed == 1 && serverListener.m_closed == 1 && clientListener.m_reason == 19);
//        SOCKET_CHECK(clientListener.m_valid && serverListener.m_valid && client.peerCount() == 0 && server.peerCount() == 0);
//        client.shutdown();
//        server.shutdown();
//    }
//#endif
//    // pauseRead允许同步报告EOF：已经收到的数据必须先于唯一终态交付。
//    {
//        NetHostSocketTestDriver driver;
//        NetHostFunctions functions = driver.functions();
//        NetHostSocketTestListener listener;
//        listener.m_trace = true;
//        NetHostSocket socket(&functions);
//        listener.m_socket = &socket;
//        uint64_t peer = 0;
//        SOCKET_CHECK(socket.connect("127.0.0.1", 14327, &listener, peer) == NI_OK);
//        driver.m_connected(driver.m_observer, peer);
//        socket.poll();
//        SOCKET_CHECK(socket.pauseRead(peer, false) == NI_OK);
//        const uint8_t data[] = {0x51, 0x52, 0x53};
//        driver.m_closeOnPause = true;
//        driver.m_data(driver.m_observer, peer, data, sizeof(data));
//        SOCKET_CHECK(listener.m_data == 0 && listener.m_closed == 0);
//        socket.poll();
//        SOCKET_CHECK(listener.m_data == 1 && listener.m_closed == 1 && listener.m_bytes.size() == 3 &&
//            std::memcmp(listener.m_bytes.data(), data, 3) == 0 && listener.m_events.size() == 3 &&
//            listener.m_events[0] == 1 && listener.m_events[1] == 2 && listener.m_events[2] == 4);
//        SOCKET_CHECK(socket.uninit() == NI_OK && driver.m_stops == 1 && driver.m_destroys == 1);
//    }
//    // 每个peer四类事件全部排满，不能依赖deque扩容或丢弃最后一个EOF。
//    {
//        NetHostSocketTestDriver driver;
//        NetHostFunctions functions = driver.functions();
//        NetHostSocketTestListener listener;
//        listener.m_bytes.reserve(4194304);
//        NetHostSocket socket(&functions);
//        listener.m_socket = &socket;
//        SOCKET_CHECK(socket.listen("127.0.0.1", 0, &listener, 256) == NI_OK);
//        uint8_t data[16384];
//        for (uint64_t peer = 1; peer <= 256; ++peer)
//        {
//            driver.m_connected(driver.m_observer, peer);
//            SOCKET_CHECK(socket.pauseRead(peer, false) == NI_OK);
//            std::memset(data, static_cast<uint8_t>(peer), sizeof(data));
//            driver.m_data(driver.m_observer, peer, data, sizeof(data));
//            driver.m_writable(driver.m_observer, peer);
//            driver.m_closed(driver.m_observer, peer, 0);
//        }
//        SOCKET_CHECK(listener.m_connected == 0 && listener.m_data == 0 && listener.m_closed == 0 && socket.peerCount() == 256);
//        socket.poll();
//        SOCKET_CHECK(listener.m_valid && listener.m_connected == 256 && listener.m_data == 256 && listener.m_writable == 256 &&
//            listener.m_closed == 256 && listener.m_bytes.size() == 4194304 && socket.peerCount() == 0);
//        for (uint32_t index = 0; index < listener.m_bytes.size(); ++index)
//        {
//            SOCKET_CHECK(listener.m_bytes[index] == static_cast<uint8_t>(index / 16384 + 1));
//        }
//        SOCKET_CHECK(socket.uninit() == NI_OK);
//    }
//    // 回调中恢复读取可同步带入下一块，但不得覆盖当前借用；一轮最多交付2048个事件。
//    {
//        NetHostSocketTestDriver driver;
//        NetHostFunctions functions = driver.functions();
//        NetHostSocketTestListener listener;
//        listener.m_resumeOnData = true;
//        NetHostSocket socket(&functions);
//        listener.m_socket = &socket;
//        uint64_t peer = 0;
//        SOCKET_CHECK(socket.connect("127.0.0.1", 14327, &listener, peer) == NI_OK);
//        driver.m_connected(driver.m_observer, peer);
//        socket.poll();
//        driver.m_dataOnResume = 2050;
//        SOCKET_CHECK(socket.pauseRead(peer, false) == NI_OK);
//        socket.poll();
//        SOCKET_CHECK(listener.m_data == 2048 && listener.m_valid && listener.m_closed == 0);
//        socket.poll();
//        SOCKET_CHECK(listener.m_data == 2050 && listener.m_valid && listener.m_bytes.size() == 2050 * 64);
//        for (uint32_t index = 0; index < listener.m_bytes.size(); ++index)
//        {
//            SOCKET_CHECK(listener.m_bytes[index] == static_cast<uint8_t>(2049 - index / 64));
//        }
//        socket.close(peer, NI_CANCELLED);
//        socket.poll();
//        SOCKET_CHECK(listener.m_closed == 1 && socket.uninit() == NI_OK);
//    }
//    // 函数表每个外部入口的异常均必须收敛，不留下永久执行标记。
//    for (uint32_t fault = 1; fault <= 512; fault <<= 1)
//    {
//        NetHostSocketTestDriver driver;
//        NetHostFunctions functions = driver.functions();
//        NetHostSocketTestListener listener;
//        NetHostSocket socket(&functions);
//        listener.m_socket = &socket;
//        uint64_t peer = 0;
//        if (fault <= 4)
//        {
//            driver.m_fault = fault;
//            const int32_t result = fault == 2 ? socket.listen("127.0.0.1", 0, &listener, 4) :
//                socket.connect("127.0.0.1", 14327, &listener, peer);
//            SOCKET_CHECK(result < 0 && peer == 0 && !socket.poll() && listener.m_connected == 0 && listener.m_closed == 0);
//        }
//        else
//        {
//            SOCKET_CHECK(socket.connect("127.0.0.1", 14327, &listener, peer) == NI_OK);
//            driver.m_connected(driver.m_observer, peer);
//            socket.poll();
//            driver.m_fault = fault;
//            const uint8_t data = 1;
//            if (fault == 8) { SOCKET_CHECK(!socket.poll()); }
//            if (fault == 16) { SOCKET_CHECK(socket.write(peer, &data, 1) == NI_INVALID_STATE); }
//            if (fault == 32) { SOCKET_CHECK(socket.pauseRead(peer, false) == NI_INVALID_STATE); }
//            if (fault == 64) { socket.close(peer, NI_CANCELLED); }
//            if (fault == 128) { socket.shutdown(); }
//            if (fault == 256)
//            {
//                SOCKET_CHECK(socket.uninit() == NI_INVALID_STATE && driver.m_destroys == 1);
//                driver.m_fault = 0;
//            }
//            if (fault == 512) { SOCKET_CHECK(socket.localPort() == 0); }
//            socket.shutdown();
//            SOCKET_CHECK(!socket.poll() && listener.m_closed == 1 && socket.peerCount() == 0);
//        }
//        SOCKET_CHECK(socket.uninit() == NI_OK && socket.uninit() == NI_OK && !socket.poll());
//        SOCKET_CHECK(driver.m_destroys == (fault == 1 ? 0U : fault == 256 ? 2U : 1U));
//    }
//    // 上层数据回调异常只关闭它自己的peer，不影响同宿主上的另一连接。
//    {
//        NetHostSocketTestDriver driver;
//        NetHostFunctions functions = driver.functions();
//        NetHostSocketTestListener listener;
//        NetHostSocket socket(&functions);
//        listener.m_socket = &socket;
//        SOCKET_CHECK(socket.listen("127.0.0.1", 0, &listener, 2) == NI_OK);
//        driver.m_connected(driver.m_observer, 1);
//        driver.m_connected(driver.m_observer, 2);
//        socket.poll();
//        listener.m_throwOnData = true;
//        SOCKET_CHECK(socket.pauseRead(1, false) == NI_OK);
//        const uint8_t byte = 0x34;
//        driver.m_data(driver.m_observer, 1, &byte, 1);
//        socket.poll();
//        SOCKET_CHECK(listener.m_data == 1 && listener.m_closed == 1 && socket.peerCount() == 1);
//        listener.m_throwOnData = false;
//        SOCKET_CHECK(socket.pauseRead(2, false) == NI_OK);
//        driver.m_data(driver.m_observer, 2, &byte, 1);
//        socket.poll();
//        SOCKET_CHECK(listener.m_data == 2 && listener.m_closed == 1 && socket.status() == NI_OK);
//        SOCKET_CHECK(socket.uninit() == NI_OK && listener.m_closed == 2);
//    }
//
//#if defined(_WIN32) && defined(_DEBUG)
//    // 下一次分配可发生在复制输入，但已预热的派发与结束路径不能再分配。
//    for (uint32_t mode = 0; mode < 3; ++mode)
//    {
//        NetHostSocketTestDriver driver;
//        NetHostFunctions functions = driver.functions();
//        NetHostSocketTestListener listener;
//        listener.m_bytes.reserve(16384);
//        NetHostSocket socket(&functions);
//        listener.m_socket = &socket;
//        uint64_t peer = 0;
//        SOCKET_CHECK(socket.connect("127.0.0.1", 14327, &listener, peer) == NI_OK);
//        driver.m_connected(driver.m_observer, peer);
//        socket.poll();
//        SOCKET_CHECK(socket.pauseRead(peer, false) == NI_OK);
//        uint8_t bytes[128];
//        std::memset(bytes, 0x55, sizeof(bytes));
//        if (mode != 0) { driver.m_data(driver.m_observer, peer, bytes, sizeof(bytes)); }
//        g_hostAllocations = 0;
//        g_hostRejectAfter = 0;
//        const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(HostMemoryHook);
//        if (mode == 0) { driver.m_data(driver.m_observer, peer, bytes, sizeof(bytes)); }
//        if (mode == 2) { socket.shutdown(); }
//        socket.poll();
//        const int32_t stopped = socket.uninit();
//        _CrtSetAllocHook(previous);
//        g_hostRejectAfter = -1;
//        SOCKET_CHECK(stopped == NI_OK && listener.m_closed == 1 && listener.m_data == (mode == 1 ? 1U : 0U));
//        SOCKET_CHECK(mode == 0 ? g_hostAllocations == 1 && socket.status() == NI_NO_MEMORY : g_hostAllocations == 0);
//    }
//    // 连续收包的两块交换缓冲只在预热分配，当前借用和下一块各自拥有。
//    {
//        NetHostSocketTestDriver driver;
//        NetHostFunctions functions = driver.functions();
//        NetHostSocketTestListener listener;
//        listener.m_bytes.reserve(16384);
//        NetHostSocket socket(&functions);
//        listener.m_socket = &socket;
//        uint64_t peer = 0;
//        SOCKET_CHECK(socket.connect("127.0.0.1", 14327, &listener, peer) == NI_OK);
//        driver.m_connected(driver.m_observer, peer);
//        socket.poll();
//        uint8_t bytes[16384];
//        std::memset(bytes, 0x6d, sizeof(bytes));
//        for (uint32_t index = 0; index < 4; ++index)
//        {
//            socket.pauseRead(peer, false);
//            driver.m_data(driver.m_observer, peer, bytes, sizeof(bytes));
//            socket.poll();
//            listener.m_bytes.clear();
//        }
//        g_hostAllocations = 0;
//        g_hostRejectAfter = 0;
//        const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(HostMemoryHook);
//        for (uint32_t index = 0; index < 10000; ++index)
//        {
//            socket.pauseRead(peer, false);
//            driver.m_data(driver.m_observer, peer, bytes, sizeof(bytes));
//            socket.poll();
//            listener.m_bytes.clear();
//        }
//        _CrtSetAllocHook(previous);
//        g_hostRejectAfter = -1;
//        SOCKET_CHECK(g_hostAllocations == 0 && listener.m_data == 10004 && listener.m_closed == 0 && socket.status() == NI_OK);
//        SOCKET_CHECK(socket.uninit() == NI_OK);
//    }
//#endif
//
//    }
//#if defined(_WIN32) && defined(_DEBUG)
//    _CrtMemState after;
//    _CrtMemCheckpoint(&after);
//    SOCKET_CHECK(after.lCounts[_NORMAL_BLOCK] == before.lCounts[_NORMAL_BLOCK] && after.lSizes[_NORMAL_BLOCK] == before.lSizes[_NORMAL_BLOCK]);
//#endif
//    std::printf("Host socket checks=%u PASS\n", assertions);
//    return 0;
//#undef SOCKET_CHECK
//}
// NET_HOST_SOCKET_UNIT_TEST_END