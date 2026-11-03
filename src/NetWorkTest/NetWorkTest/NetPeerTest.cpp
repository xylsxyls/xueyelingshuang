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

NetPeerTest::NetPeerTest() :
m_closed(0),
m_other(0),
m_generation(0), m_client(nullptr), m_cpp(nullptr), m_reenter(nullptr), m_throwEvents(0), m_bad(false)
{

}

void NetPeerTest::onSessionReady(uint64_t generation, uint32_t requests, uint32_t chunk)
{
    (void)generation; (void)requests; (void)chunk; ++m_other; if (m_reenter) { m_reenter(*this); } if (m_throwEvents & 1) { throw 1; }
}

void NetPeerTest::onSessionResult(uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    (void)generation; (void)product; (void)type; (void)status; m_resultIds.push_back(id); m_resultBytes.push_back(size == 0 ? 0 : bytes[0]); ++m_other; if (m_reenter) { m_reenter(*this); } if (m_throwEvents & 2) { throw 2; }
}

bool NetPeerTest::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    (void)generation; (void)frame; ++m_other; if (m_reenter) { m_reenter(*this); } if (m_throwEvents & 4) { throw 4; } return false;
}

void NetPeerTest::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    (void)reason; (void)detail; ++m_closed; m_generation = generation; if (m_reenter) { m_reenter(*this); } if (m_throwEvents & 8) { throw 8; }
}

int32_t NetPeerTest::entropy(void* context, uint8_t* bytes, uint32_t size)
{
    (void)context; std::memset(bytes, 0x71, size); return 0;
}

NetHostFunctions NetPeerTest::functions()
{
    NetHostFunctions value = {};
    value.structSize = sizeof(value); value.version = 1;
    value.create = create; value.connect = connect; value.poll = operation; value.write = write;
    value.pauseRead = control; value.close = control; value.shutdown = operation; value.destroy = operation;
    value.localPort = query; value.peerCount = query; value.entropy = entropy;
    return value;
}

int32_t NetPeerTest::create(void* context, void* observer, NetHostConnected connected, NetHostData data, NetHostWritable writable, NetHostClosed closed, void** instance)
{
    ++m_created;
    (void)context; (void)observer; (void)connected; (void)data; (void)writable; (void)closed; *instance = nullptr; return -1;
}

int32_t NetPeerTest::connect(void* instance, const char* address, uint32_t port, uint64_t* peer)
{
    (void)instance; (void)address; (void)port; *peer = 0; return -1;
}

int32_t NetPeerTest::operation(void* instance)
{
    (void)instance; return 0;
}

int32_t NetPeerTest::write(void* instance, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    (void)instance; (void)peer; (void)bytes; (void)size; return -1;
}

int32_t NetPeerTest::control(void* instance, uint64_t peer, int32_t value)
{
    (void)instance; (void)peer; (void)value; return 0;
}

uint32_t NetPeerTest::query(void* instance)
{
    (void)instance; return 0;
}

void NetPeerTest::ready(void* context, uint64_t generation, uint32_t requests, uint32_t chunk)
{
    static_cast<NetPeerTest*>(context)->onSessionReady(generation, requests, chunk);
}

void NetPeerTest::result(void* context, uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    static_cast<NetPeerTest*>(context)->onSessionResult(generation, id, product, type, status, bytes, size);
}

int32_t NetPeerTest::frame(void* context, uint64_t generation, const NetInterfaceFrame* frame)
{
    return static_cast<NetPeerTest*>(context)->onSessionFrame(generation, *frame) ? 1 : 0;
}

void NetPeerTest::closed(void* context, uint64_t generation, int32_t reason, int32_t detail)
{
    static_cast<NetPeerTest*>(context)->onSessionClosed(generation, reason, detail);
}

NetPeerClientOptions NetPeerTest::options()
{
    NetPeerClientOptions value = {};
    value.structSize = sizeof(value);
    value.version = 1;
    value.address = "127.0.0.1";
    value.port = 1;
    value.platform = 1;
    value.capabilities = 7;
    value.maxRequests = 4;
    value.maxChunk = 4096;
    value.handshakeTimeoutMs = 1000;
    value.tls.handshakeTimeoutMs = 1000;
    value.tls.entropy = entropy;
    std::memset(value.device, 0x31, sizeof(value.device));
    return value;
}

NetPeerClientCallbacks NetPeerTest::callbacks()
{
    NetPeerClientCallbacks value = {};
    value.structSize = sizeof(value);
    value.version = 1;
    value.context = this;
    value.ready = ready;
    value.result = result;
    value.frame = frame;
    value.closed = closed;
    return value;
}

void NetPeerTest::inspectC(NetPeerTest& test)
{
    const NetPeerClientOptions options = NetPeerTest::options();
    uint64_t ticket = 99;
    NetInterfaceFrame frame = {};
    test.m_bad = test.m_bad || NetPeer::instance().clientDestroy(test.m_client) != NI_INVALID_STATE ||
        NetPeer::instance().clientPoll(test.m_client) != NI_INVALID_STATE ||
        NetPeer::instance().clientConnect(test.m_client, &options) != NI_INVALID_STATE ||
        NetPeer::instance().clientConnectService(test.m_client, 101, &options) != NI_INVALID_STATE ||
        NetPeer::instance().clientConfigureCompression(test.m_client, nullptr, 0) != NI_INVALID_STATE ||
        NetPeer::instance().clientConfigureGateways(test.m_client, nullptr, nullptr) != NI_INVALID_STATE ||
        NetPeer::instance().clientSend(test.m_client, 1, nullptr, 0) != NI_INVALID_STATE ||
        NetPeer::instance().clientSendData(test.m_client, &frame) != NI_INVALID_STATE ||
        NetPeer::instance().clientSendBegin(test.m_client, 1, nullptr, 0, &ticket) != NI_INVALID_STATE || ticket != 0;
}