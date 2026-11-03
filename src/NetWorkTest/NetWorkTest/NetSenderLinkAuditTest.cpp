#include "NetSender/NetSenderAPI.h"
#if defined(_WIN32) && defined(_DEBUG)
std::atomic<uint32_t> NetSenderGrantAudit::m_thread(0);
int NetSenderGrantAudit::m_failAfter = -1;

#endif
#include "NetLinkLocalTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>
#include <cstdio>
#include "NetLinkTestCases.h"

#ifndef __EMSCRIPTEN__
NetSenderLinkAuditListener::NetSenderLinkAuditListener() :
m_state(nullptr),
m_mode(0),
m_attempts(0),
m_records(0),
m_controls(0),
m_closed(0),
m_reason(0),
m_valid(true)
{

}
#endif

#ifndef __EMSCRIPTEN__
void NetSenderLinkAuditListener::onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity)
{
    m_valid = m_valid && peer == 1 && identity.m_role == NI_LINK_NODE;
}
#endif

#ifndef __EMSCRIPTEN__
bool NetSenderLinkAuditListener::onLinkControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    const uint8_t expected[] = {9, 7, 5, 3};
    m_valid = m_valid && peer == 1 && frame.m_messageType == 991 && frame.m_payloadSize == sizeof(expected) &&
        std::memcmp(frame.m_payload, expected, sizeof(expected)) == 0;
    ++m_controls;
    return true;
}
#endif

#ifndef __EMSCRIPTEN__
bool NetSenderLinkAuditListener::onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    const uint8_t expected[] = {9, 7, 5, 3};
    m_valid = m_valid && peer == 1 && record.m_product == 7 && record.m_channel == 1 && record.m_size == sizeof(expected) &&
        std::memcmp(record.m_data, expected, sizeof(expected)) == 0;
    ++m_attempts;
    if (m_mode == 3 && m_attempts <= 3)
    {
        return false;
    }
    ++m_records;
    if (m_mode == 5)
    {
        m_state->close(peer, NI_CANCELLED);
    }
    return true;
}
#endif

#ifndef __EMSCRIPTEN__
void NetSenderLinkAuditListener::onLinkClosed(uint64_t peer, int32_t reason)
{
    ++m_closed;
    m_reason = reason;
    m_valid = m_valid && m_state->m_peers.find(peer) == m_state->m_peers.end();
}
#endif

NetSenderGrantAudit::NetSenderGrantAudit() :
m_ready(false),
m_closed(false),
m_valid(true),
m_peer(0)
{

}

void NetSenderGrantAudit::onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity)
{
    m_valid = m_valid && !m_ready && peer == m_peer && identity.m_role == NI_LINK_NODE;
    m_ready = true;
}

bool NetSenderGrantAudit::onLinkControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    (void)frame;
    m_valid = m_valid && peer == m_peer;
    return true;
}

bool NetSenderGrantAudit::onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    (void)peer;
    (void)record;
    m_valid = false;
    return true;
}

void NetSenderGrantAudit::onLinkClosed(uint64_t peer, int32_t reason)
{
    (void)reason;
    m_valid = m_valid && peer == m_peer && !m_closed;
    m_closed = true;
}

#if defined(_WIN32) && defined(_DEBUG)
int NetSenderGrantAudit::allocation(int type, void* data, size_t size, int block, long request,
    const unsigned char* file, int line)
{
    (void)data;
    (void)size;
    (void)request;
    (void)file;
    (void)line;
    if (type == _HOOK_ALLOC && block == _NORMAL_BLOCK && GetCurrentThreadId() == m_thread && m_failAfter >= 0)
    {
        if (m_failAfter == 0)
        {
            return FALSE;
        }
        --m_failAfter;
    }
    return TRUE;
}
#endif

int NetSenderGrantAudit::run(int32_t pid, NetInterfaceLinkServiceProof proof)
{
    NetSenderLocalInbox inbox;
    NetSenderGrantAudit probe;
    NetSenderLink link(inbox);
    ProcessWork::instance().addProcessReceiveCallback(&inbox);
    if (!ProcessWork::instance().initReceive(16452, 32))
    {
        ProcessWork::instance().uninitReceive();
        return 1;
    }
    bool valid = false;
    uint32_t rejected = 0;
    std::string key;
    try
    {
        const std::string name = "service.7";
        std::string request;
        valid = NetInterfaceCertificate::createRequest(name, CSystem::secureRandomBytesCallback, nullptr,
            key, request) == NI_OK;
        NetInterfaceLinkOptions options = {};
        options.m_version = 3;
        options.m_role = NI_LINK_SERVICE;
        options.m_name = name.c_str();
        options.m_tls.privateKey = reinterpret_cast<const uint8_t*>(key.c_str());
        options.m_tls.privateKeySize = static_cast<uint32_t>(key.size() + 1);
        options.m_tls.handshakeTimeoutMs = 5000;
        options.m_claims = reinterpret_cast<const uint8_t*>(request.data());
        options.m_claimsSize = static_cast<uint32_t>(request.size());
        options.m_serviceProof = proof;
        valid = valid && link.connect(pid, options, &probe, probe.m_peer) == NI_OK;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (valid && !probe.m_ready && !probe.m_closed && std::chrono::steady_clock::now() < deadline)
        {
            link.poll();
            CSystem::Sleep(1);
        }
        std::string expected;
        valid = valid && probe.m_ready && probe.m_valid && !probe.m_closed &&
            link.serviceGrant(probe.m_peer, expected) == NI_READY && expected.size() > 128 &&
            NetInterfaceCertificate::verifyPair(expected, key, name, CSystem::secureRandomBytesCallback, nullptr) == NI_OK;
        for (int position = 0; valid && position < 8; ++position)
        {
            std::string output;
            int32_t result = NI_INVALID_STATE;
            bool escaped = false;
#if defined(_WIN32) && defined(_DEBUG)
            m_thread = GetCurrentThreadId();
            m_failAfter = position;
            _CRT_ALLOC_HOOK old = _CrtSetAllocHook(allocation);
#endif
            try
            {
                result = link.serviceGrant(probe.m_peer, output);
            }
            catch (...)
            {
                escaped = true;
            }
#if defined(_WIN32) && defined(_DEBUG)
            _CrtSetAllocHook(old);
            m_failAfter = -1;
#endif
            valid = !escaped && ((result == NI_NO_MEMORY && output.empty()) || (result == NI_READY && output == expected));
            if (result == NI_NO_MEMORY)
            {
                ++rejected;
            }
            valid = valid && link.serviceGrant(probe.m_peer, output) == NI_READY && output == expected;
        }
#if defined(_WIN32) && defined(_DEBUG)
        valid = valid && rejected != 0;
#endif
        std::string invalid = "old result";
        valid = valid && link.serviceGrant(probe.m_peer + 1, invalid) == NI_INVALID_STATE && invalid.empty();
        link.close(probe.m_peer);
        invalid = "old result";
        valid = valid && link.serviceGrant(probe.m_peer, invalid) == NI_INVALID_STATE && invalid.empty();
    }
    catch (...)
    {
        valid = false;
    }
    if (!key.empty())
    {
        NetInterface::wipe(reinterpret_cast<uint8_t*>(&key[0]), static_cast<uint32_t>(key.size()));
    }
    link.shutdown();
    ProcessWork::instance().uninitReceive();
    valid = valid && probe.m_valid && probe.m_closed;
    std::printf("Service grant actual IPC positions=8 rejected=%u %s\n", rejected, valid ? "PASS" : "FAIL");
    return valid ? 0 : 1;
}