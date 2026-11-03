#include "NetPeer/NetPeerAPI.h"
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

NetHostSocketTestDriver::NetHostSocketTestDriver() :
m_observer(nullptr),
m_connected(nullptr),
m_data(nullptr),
m_writable(nullptr),
m_closed(nullptr),
m_writeResult(0),
m_pauseResult(0),
m_entropyMode(0),
m_destroyFailures(0),
m_active(false),
m_syncConnect(false),
m_createFail(false),
m_fault(0),
m_closeOnPause(false),
m_dataOnResume(0),
m_written(0),
m_pauses(0),
m_stops(0),
m_destroys(0)
{

}

NetHostFunctions NetHostSocketTestDriver::functions()
{
    NetHostFunctions functions = {};
    functions.structSize = sizeof(functions);
    functions.version = 1;
    functions.context = this;
    functions.create = create;
    functions.listen = listen;
    functions.connect = connect;
    functions.poll = poll;
    functions.write = write;
    functions.pauseRead = pauseRead;
    functions.close = close;
    functions.shutdown = shutdown;
    functions.destroy = destroy;
    functions.localPort = localPort;
    functions.peerCount = peerCount;
    functions.entropy = entropy;
    return functions;
}

int32_t NetHostSocketTestDriver::create(void* providerContext, void* observerContext, NetHostConnected connected,
    NetHostData data, NetHostWritable writable, NetHostClosed closed, void** instance)
{
    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(providerContext);
    *instance = nullptr;
    if ((driver->m_fault & 1) != 0) { throw 1; }
    if (driver->m_createFail)
    {
        return -1;
    }
    driver->m_observer = observerContext;
    driver->m_connected = connected;
    driver->m_data = data;
    driver->m_writable = writable;
    driver->m_closed = closed;
    *instance = driver;
    return 0;
}

int32_t NetHostSocketTestDriver::listen(void* instance, const char* address, uint32_t port, uint32_t maxPeers)
{
    (void)address;
    (void)port;
    (void)maxPeers;
    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
    if ((driver->m_fault & 2) != 0) { throw 1; }
    driver->m_active = true;
    return 0;
}

int32_t NetHostSocketTestDriver::connect(void* instance, const char* address, uint32_t port, uint64_t* peer)
{
    (void)address;
    (void)port;
    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
    if ((driver->m_fault & 4) != 0) { throw 1; }
    *peer = 1;
    driver->m_active = true;
    if (driver->m_syncConnect)
    {
        driver->m_connected(driver->m_observer, 1);
    }
    return 0;
}

int32_t NetHostSocketTestDriver::poll(void* instance)
{
    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
    if ((driver->m_fault & 8) != 0) { throw 1; }
    return driver->m_active ? 1 : 0;
}

int32_t NetHostSocketTestDriver::write(void* instance, uint64_t peer, const uint8_t* data, uint32_t size)
{
    (void)peer;
    (void)data;
    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
    if ((driver->m_fault & 16) != 0) { throw 1; }
    if (driver->m_writeResult == 0)
    {
        driver->m_written += size;
    }
    return driver->m_writeResult;
}

int32_t NetHostSocketTestDriver::pauseRead(void* instance, uint64_t peer, int32_t paused)
{
    (void)peer;
    (void)paused;
    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
    ++driver->m_pauses;
    if ((driver->m_fault & 32) != 0) { throw 1; }
    if (paused != 0 && driver->m_closeOnPause)
    {
        driver->m_closeOnPause = false;
        driver->m_closed(driver->m_observer, peer, 0);
    }
    if (paused == 0 && driver->m_dataOnResume != 0)
    {
        --driver->m_dataOnResume;
        uint8_t bytes[64];
        std::memset(bytes, static_cast<uint8_t>(driver->m_dataOnResume), sizeof(bytes));
        driver->m_data(driver->m_observer, peer, bytes, sizeof(bytes));
        std::memset(bytes, 0, sizeof(bytes));
    }
    return driver->m_pauseResult;
}

int32_t NetHostSocketTestDriver::close(void* instance, uint64_t peer, int32_t reason)
{
    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
    if ((driver->m_fault & 64) != 0) { throw 1; }
    driver->m_closed(driver->m_observer, peer, reason);
    return 0;
}

int32_t NetHostSocketTestDriver::shutdown(void* instance)
{
    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
    driver->m_active = false;
    ++driver->m_stops;
    if ((driver->m_fault & 128) != 0) { throw 1; }
    return 0;
}

int32_t NetHostSocketTestDriver::destroy(void* instance)
{
    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(instance);
    ++driver->m_destroys;
    if ((driver->m_fault & 256) != 0) { throw 1; }
    if (driver->m_destroyFailures != 0)
    {
        --driver->m_destroyFailures;
        return -1;
    }
    return 0;
}

uint32_t NetHostSocketTestDriver::localPort(void* instance)
{
    (void)instance;
    if ((static_cast<NetHostSocketTestDriver*>(instance)->m_fault & 512) != 0) { throw 1; }
    return 14327;
}

uint32_t NetHostSocketTestDriver::peerCount(void* instance)
{
    return static_cast<NetHostSocketTestDriver*>(instance)->m_active ? 1 : 0;
}

int32_t NetHostSocketTestDriver::entropy(void* context, uint8_t* output, uint32_t size)
{
    NetHostSocketTestDriver* driver = static_cast<NetHostSocketTestDriver*>(context);
    if (driver->m_entropyMode != 0)
    {
        if (size != 0)
        {
            output[0] = 0x7c;
        }
        if (driver->m_entropyMode == 2)
        {
            throw 1;
        }
        return -1;
    }
    std::memset(output, 0x6d, size);
    return 0;
}

NetHostSocketTestListener::NetHostSocketTestListener() :
m_socket(nullptr),
m_connected(0),
m_data(0),
m_writable(0),
m_closed(0),
m_reason(0),
m_detail(0),
m_stopOnConnect(false),
m_throwOnConnect(false),
m_resumeOnData(false),
m_throwOnData(false),
m_trace(false),
m_valid(true)
{

}

void NetHostSocketTestListener::onConnected(uint64_t peer)
{
    ++m_connected;
    if (m_trace) { m_events.push_back(1); }
    m_valid = m_valid && peer != 0;
    m_socket->poll();
    m_valid = m_valid && m_socket->uninit() == NI_INVALID_STATE;
    if (m_stopOnConnect)
    {
        m_socket->shutdown();
    }
    if (m_throwOnConnect)
    {
        throw 1;
    }
}

void NetHostSocketTestListener::onData(uint64_t peer, const uint8_t* data, uint32_t size)
{
    ++m_data;
    if (m_trace) { m_events.push_back(2); }
    m_valid = m_valid && peer != 0 && data != nullptr && size != 0 && size <= 16384;
    const size_t begin = m_bytes.size();
    m_bytes.insert(m_bytes.end(), data, data + size);
    if (m_resumeOnData)
    {
        m_valid = m_valid && m_socket->pauseRead(peer, false) == NI_OK;
        m_valid = m_valid && std::memcmp(data, &m_bytes[begin], size) == 0;
    }
    if (m_throwOnData) { throw 1; }
}

void NetHostSocketTestListener::onWritable(uint64_t peer)
{
    m_valid = m_valid && peer != 0;
    ++m_writable;
    if (m_trace) { m_events.push_back(3); }
}

void NetHostSocketTestListener::onClosed(uint64_t peer, int32_t reason, int32_t detail)
{
    m_valid = m_valid && peer != 0;
    ++m_closed;
    if (m_trace) { m_events.push_back(4); }
    m_reason = reason;
    m_detail = detail;
    m_socket->poll();
}