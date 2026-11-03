#include "NetSenderLink.h"
#include "NetSenderMacro.h"
#include "NetSenderLinkState.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>

NetSenderLink::NetSenderLink(NetSenderLocalInbox& inbox) : m_state(new NetSenderLinkState(inbox))
{

}

NetSenderLink::~NetSenderLink()
{
    shutdown();
    delete m_state;
}

int32_t NetSenderLink::listen(const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener, uint32_t maximum)
{
    if (options.m_tls.server != 1) { return NI_INVALID_ARGUMENT; }
    const int32_t configured = m_state->configure(options, listener);
    return configured == NI_OK ? m_state->m_raw.listenRaw(m_state->m_options.m_tls.entropy,
        m_state->m_options.m_tls.entropyContext, m_state, maximum) : configured;
}

int32_t NetSenderLink::connect(int32_t pid, const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener, uint64_t& peer)
{
    peer = 0;
    if (options.m_tls.server != 0) { return NI_INVALID_ARGUMENT; }
    const int32_t configured = m_state->configure(options, listener);
    if (configured != NI_OK) { return configured; }
    const int32_t connected = m_state->m_raw.connectRaw(pid, m_state->m_options.m_tls.entropy,
        m_state->m_options.m_tls.entropyContext, m_state, peer);
    if (connected != NI_OK) { return connected; }
    try
    {
        std::unique_ptr<NetSenderLinkPeer> entry(new NetSenderLinkPeer(m_state->m_options));
        const int32_t status = entry->m_link.status();
        m_state->m_peers.insert(std::make_pair(peer, std::move(entry)));
        if (status < 0) { m_state->close(peer, status); peer = 0; return status; }
        return NI_OK;
    }
    catch (...)
    {
        m_state->m_raw.close(peer, NI_NO_MEMORY);
        peer = 0;
        return NI_NO_MEMORY;
    }
}

bool NetSenderLink::poll()
{
    if (m_state->m_polling) { return !m_state->m_stopping; }
    m_state->m_polling = true;
    const bool active = m_state->m_raw.poll();
    for (auto& entry : m_state->m_peers)
    {
        try { m_state->pump(entry.first, *entry.second); }
        catch (...) { m_state->close(entry.first, NI_NO_MEMORY); }
    }
    m_state->collectClosed();
    m_state->m_polling = false;
    if (m_state->m_stopping) { shutdown(); }
    return (active || !m_state->m_peers.empty()) && !m_state->m_stopping;
}

int32_t NetSenderLink::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    auto found = m_state->m_peers.find(peer);
    return m_state->m_stopping || found == m_state->m_peers.end() || (found->second->m_closing || found->second->m_closed) ? NI_INVALID_STATE :
        found->second->m_link.sendControl(frame);
}

int32_t NetSenderLink::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    auto found = m_state->m_peers.find(peer);
    return m_state->m_stopping || found == m_state->m_peers.end() || (found->second->m_closing || found->second->m_closed) ? NI_INVALID_STATE :
        found->second->m_link.sendRecord(record);
}

int32_t NetSenderLink::serviceGrant(uint64_t peer, std::string& certificate) const
{
    certificate.clear();
    auto found = m_state->m_peers.find(peer);
    if (m_state->m_stopping || found == m_state->m_peers.end() || found->second->m_closing) { return NI_INVALID_STATE; }
    const uint8_t* bytes = nullptr;
    uint32_t size = 0;
    const int32_t ready = found->second->m_link.serviceGrant(bytes, size);
    if (ready == NI_READY)
    {
        try
        {
            certificate.assign(reinterpret_cast<const char*>(bytes), size);
        }
        catch (...)
        {
            certificate.clear();
            return NI_NO_MEMORY;
        }
    }
    return ready;
}

void NetSenderLink::close(uint64_t peer, int32_t reason)
{
    m_state->close(peer, reason);
}

void NetSenderLink::shutdown()
{
    m_state->m_stopping = true;
    if (m_state->m_polling) { return; }
    m_state->m_polling = true;
    for (auto& entry : m_state->m_peers) { m_state->close(entry.first, 0); }
    m_state->m_raw.shutdown();
    m_state->collectClosed();
    m_state->m_polling = false;
}

int32_t NetSenderLink::outputState(uint64_t peer) const
{
    const auto found = m_state->m_peers.find(peer);
    if (found == m_state->m_peers.end() || found->second->m_closing || found->second->m_closed) { return NI_INVALID_STATE; }
    const int32_t local = found->second->m_link.outputState();
    return local == NI_OK ? m_state->m_raw.outputState(peer) : local;
}

// NET_SENDER_LINK_UNIT_BEGIN
// #include "NetInterface/NetInterfaceAPI.h"
// #include <cstdio>
// #include <cstring>
// #include <vector>
// #if defined(_WIN32) && defined(_DEBUG)
// #include <crtdbg.h>
// #endif
//
// NetInterfaceLinkTestIdentity::NetInterfaceLinkTestIdentity() :
// m_seed(0x47b9af13)
// {
//
// }
//
// NetInterfaceLinkTestIdentity::~NetInterfaceLinkTestIdentity()
// {
//     NetInterface::wipe(m_key.empty() ? nullptr : reinterpret_cast<uint8_t*>(&m_key[0]), static_cast<uint32_t>(m_key.size()));
// }
//
// bool NetInterfaceLinkTestIdentity::load(const char* path)
// {
//     FILE* file = std::fopen(path, "rb");
//     if (file == nullptr)
//     {
//         return false;
//     }
//     uint8_t bytes[24577] = {};
//     const size_t size = std::fread(bytes, 1, sizeof(bytes), file);
//     const bool readable = std::ferror(file) == 0 && size >= 48 && size <= 24576 && std::memcmp(bytes, "NGI3", 4) == 0;
//     std::fclose(file);
//     bool valid = false;
//     if (readable)
//     {
//         uint32_t lengths[3] = {};
//         for (uint32_t index = 0; index < 3; ++index)
//         {
//             for (uint32_t offset = 0; offset < 4; ++offset)
//             {
//                 lengths[index] = (lengths[index] << 8) | bytes[4 + index * 4 + offset];
//             }
//         }
//         uint8_t digest[32] = {};
//         if (lengths[0] == 37 && lengths[1] > 0 && lengths[1] <= 4096 && lengths[2] > 0 && lengths[2] <= 16384 &&
//             static_cast<uint64_t>(lengths[0]) + lengths[1] + lengths[2] + 48 == size &&
//             NetInterface::sha256(bytes, static_cast<uint32_t>(size) - 32, digest, sizeof(digest)) == NI_OK &&
//             std::memcmp(bytes + size - 32, digest, sizeof(digest)) == 0)
//         {
//             try
//             {
//                 m_name.assign(reinterpret_cast<const char*>(bytes + 16), lengths[0]);
//                 m_key.assign(reinterpret_cast<const char*>(bytes + 16 + lengths[0]), lengths[1]);
//                 m_certificate.assign(reinterpret_cast<const char*>(bytes + 16 + lengths[0] + lengths[1]), lengths[2]);
//                 valid = true;
//             }
//             catch (...)
//             {
//                 valid = false;
//             }
//         }
//     }
//     NetInterface::wipe(bytes, sizeof(bytes));
//     return valid;
// }
//
// int32_t NetInterfaceLinkTestIdentity::entropy(void* context, uint8_t* output, uint32_t size)
// {
//     NetInterfaceLinkTestIdentity* identity = static_cast<NetInterfaceLinkTestIdentity*>(context);
//     for (uint32_t index = 0; index < size; ++index)
//     {
//         identity->m_seed ^= identity->m_seed << 13;
//         identity->m_seed ^= identity->m_seed >> 17;
//         identity->m_seed ^= identity->m_seed << 5;
//         output[index] = static_cast<uint8_t>(identity->m_seed);
//     }
//     return 0;
// }
//
// NetInterfaceLinkOptions NetInterfaceLinkTestIdentity::options(bool server)
// {
//     NetInterfaceLinkOptions value = {};
//     value.m_version = 3;
//     value.m_role = server ? NI_LINK_NODE : NI_LINK_CLIENT;
//     value.m_name = server ? m_name.c_str() : nullptr;
//     value.m_peerRoles = server ? 1U << NI_LINK_CLIENT : 0;
//     value.m_tls.server = server ? 1 : 0;
//     value.m_tls.handshakeTimeoutMs = 5000;
//     value.m_tls.certificate = server ? reinterpret_cast<const uint8_t*>(m_certificate.c_str()) : nullptr;
//     value.m_tls.certificateSize = server ? static_cast<uint32_t>(m_certificate.size() + 1) : 0;
//     value.m_tls.privateKey = server ? reinterpret_cast<const uint8_t*>(m_key.c_str()) : nullptr;
//     value.m_tls.privateKeySize = server ? static_cast<uint32_t>(m_key.size() + 1) : 0;
//     value.m_tls.entropy = entropy;
//     value.m_tls.entropyContext = this;
//     return value;
// }
//
// NetInterfaceLinkTestPair::NetInterfaceLinkTestPair(NetInterfaceLinkTestIdentity& identity) :
// m_client(identity.options(false)),
// m_server(identity.options(true))
// {
//
// }
//
// bool NetInterfaceLinkTestPair::handshake()
// {
//     for (uint32_t step = 0; step < 10000; ++step)
//     {
//         if (m_client.progress(1000) < 0 || m_server.progress(1000) < 0 ||
//             move(m_client, m_server) < 0 || move(m_server, m_client) < 0)
//         {
//             return false;
//         }
//         if (m_client.status() == NI_READY && m_server.status() == NI_READY)
//         {
//             return true;
//         }
//     }
//     return false;
// }
//
// int32_t NetInterfaceLinkTestPair::move(NetInterfaceLink& from, NetInterfaceLink& to)
// {
//     NetInterfaceTunnelRecord record = {};
//     const int32_t ready = from.peekOutput(record);
//     if (ready != NI_READY)
//     {
//         return ready;
//     }
//     const int32_t accepted = to.accept(record);
//     if (accepted != NI_OK)
//     {
//         return accepted == NI_LIMIT_EXCEEDED ? NI_NEED_MORE : accepted;
//     }
//     return from.consumeOutput();
// }
//
// #include "NetSender.h"
// #include "NetSender/NetSenderLinkAuditTest.h"
//
// NetSenderLinkAuditListener::NetSenderLinkAuditListener() :
// m_state(nullptr),
// m_mode(0),
// m_attempts(0),
// m_records(0),
// m_controls(0),
// m_closed(0),
// m_reason(0),
// m_valid(true)
// {
//
// }
//
// void NetSenderLinkAuditListener::onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity)
// {
//     m_valid = m_valid && peer == 1 && identity.m_role == NI_LINK_NODE;
// }
//
// bool NetSenderLinkAuditListener::onLinkControl(uint64_t peer, const NetInterfaceFrame& frame)
// {
//     const uint8_t expected[] = {9, 7, 5, 3};
//     m_valid = m_valid && peer == 1 && frame.m_messageType == 991 && frame.m_payloadSize == sizeof(expected) &&
//         std::memcmp(frame.m_payload, expected, sizeof(expected)) == 0;
//     ++m_controls;
//     return true;
// }
//
// bool NetSenderLinkAuditListener::onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
// {
//     const uint8_t expected[] = {9, 7, 5, 3};
//     m_valid = m_valid && peer == 1 && record.m_product == 7 && record.m_channel == 1 && record.m_size == sizeof(expected) &&
//         std::memcmp(record.m_data, expected, sizeof(expected)) == 0;
//     ++m_attempts;
//     if (m_mode == 3 && m_attempts <= 3)
//     {
//         return false;
//     }
//     ++m_records;
//     if (m_mode == 5)
//     {
//         m_state->close(peer, NI_CANCELLED);
//     }
//     return true;
// }
//
// void NetSenderLinkAuditListener::onLinkClosed(uint64_t peer, int32_t reason)
// {
//     ++m_closed;
//     m_reason = reason;
//     m_valid = m_valid && m_state->m_peers.find(peer) == m_state->m_peers.end();
// }
//
// #define SENDER_LINK_CHECK(condition) do { ++checks; if (!(condition)) { std::printf("Sender link failed line=%d checks=%u\n", __LINE__, checks); return 1; } } while (0)
//
// /** 同库白盒测试原始END回调与协议排空；真实跨进程路径另由NetWorkTest联调验证
// @param [in] argc 参数数量
// @param [in] argv 第一个参数为隔离身份路径
// @return 0通过
// */
// int main(int argc, char** argv)
// {
//     uint32_t checks = 0;
//     NetInterfaceLinkTestIdentity identity;
//     SENDER_LINK_CHECK(argc == 2 && identity.load(argv[1]));
// #if defined(_WIN32) && defined(_DEBUG)
//     _CrtMemState before;
//     _CrtMemCheckpoint(&before);
// #endif
//     for (uint32_t mode = 0; mode < 6; ++mode)
//     {
//         NetSenderLocalInbox inbox;
//         NetSenderLinkAuditListener listener;
//         NetSenderLinkState state(inbox);
//         listener.m_state = &state;
//         listener.m_mode = mode;
//         SENDER_LINK_CHECK(state.configure(identity.options(false), &listener) == NI_OK);
//         state.onRawLocalReady(1, 0);
//         SENDER_LINK_CHECK(state.m_peers.size() == 1);
//         NetInterfaceLink& client = state.m_peers.begin()->second->m_link;
//         NetInterfaceLink server(identity.options(true));
//         for (uint32_t step = 0; step < 10000; ++step)
//         {
//             SENDER_LINK_CHECK(client.progress(1000) >= 0 && server.progress(1000) >= 0);
//             SENDER_LINK_CHECK(NetInterfaceLinkTestPair::move(client, server) >= 0 && NetInterfaceLinkTestPair::move(server, client) >= 0);
//             if (client.status() == NI_READY && server.status() == NI_READY)
//             {
//                 break;
//             }
//         }
//         SENDER_LINK_CHECK(client.status() == NI_READY && server.status() == NI_READY);
//         // 已认证且上层已观察ready是本白盒测试的入口条件；不启用任何进程接收器。
//         state.m_peers.begin()->second->m_announced = true;
//         NetInterfaceLinkPeer identityView = {};
//         SENDER_LINK_CHECK(client.peerInfo(identityView) == NI_READY);
//         listener.onLinkReady(1, identityView);
//         const uint8_t payload[] = {9, 7, 5, 3};
//         if (mode != 1)
//         {
//             NetInterfaceTunnelRecord record = {};
//             record.m_kind = NI_TUNNEL_BYTES;
//             record.m_product = 7;
//             record.m_channel = 1;
//             record.m_data = payload;
//             record.m_size = sizeof(payload);
//             SENDER_LINK_CHECK(server.sendRecord(record) == NI_OK);
//         }
//         if (mode != 0)
//         {
//             NetInterfaceFrame frame = {};
//             frame.m_kind = NI_EVENT;
//             frame.m_messageType = 991;
//             frame.m_payload = payload;
//             frame.m_payloadSize = sizeof(payload);
//             SENDER_LINK_CHECK(server.sendControl(frame) == NI_OK);
//         }
//         for (uint32_t step = 0; step < 16; ++step)
//         {
//             NetInterfaceTunnelRecord record = {};
//             const int32_t ready = server.peekOutput(record);
//             if (ready != NI_READY)
//             {
//                 SENDER_LINK_CHECK(ready == NI_NEED_MORE);
//                 break;
//             }
//             SENDER_LINK_CHECK(state.onRawLocalRecord(1, record));
//             SENDER_LINK_CHECK(server.consumeOutput() == NI_OK);
//         }
//         state.onRawLocalClosed(1, mode == 4 ? NI_INVALID_STATE : NI_OK);
//         SENDER_LINK_CHECK(state.m_peers.size() == 1 && listener.m_closed == 0);
//         for (uint32_t step = 0; step < 16 && !state.m_peers.empty(); ++step)
//         {
//             state.pump(1, *state.m_peers.begin()->second);
//             state.collectClosed();
//         }
//         SENDER_LINK_CHECK(state.m_peers.empty() && listener.m_closed == 1 && listener.m_valid);
//         SENDER_LINK_CHECK(listener.m_records == (mode == 1 ? 0U : 1U) && listener.m_controls == (mode == 0 ? 0U : 1U));
//         SENDER_LINK_CHECK(listener.m_reason == (mode == 4 ? NI_INVALID_STATE : mode == 5 ? NI_CANCELLED : NI_TRUNCATED));
//         SENDER_LINK_CHECK(mode != 3 || listener.m_attempts == 4);
//         state.onRawLocalClosed(1, NI_TIMEOUT);
//         state.collectClosed();
//         SENDER_LINK_CHECK(listener.m_closed == 1);
//     }
// #if defined(_WIN32) && defined(_DEBUG)
//     _CrtMemState after;
//     _CrtMemState difference;
//     _CrtMemCheckpoint(&after);
//     _CrtMemDifference(&difference, &before, &after);
//     SENDER_LINK_CHECK(difference.lCounts[_NORMAL_BLOCK] == 0 && difference.lSizes[_NORMAL_BLOCK] == 0);
// #endif
//     std::printf("Sender link drain checks=%u PASS\n", checks);
//     return 0;
// }
// NET_SENDER_LINK_UNIT_END

#if 0 // NET_SERVICE_GRANT_UNIT_BEGIN
#include "NetSender.h"
#include "NetSender/NetSenderLinkAuditTest.h"
#include "CSystem/CSystemAPI.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>
#if defined(_WIN32) && defined(_DEBUG)
#include <windows.h>
#include <crtdbg.h>
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
std::atomic<uint32_t> NetSenderGrantAudit::m_thread(0);
int NetSenderGrantAudit::m_failAfter = -1;

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

#ifndef NET_SERVICE_GRANT_NO_MAIN
// 测试EXE额外链接服务器的NetServerEnrollment.cpp；凭证实现不加入NetSender正式库。
#include "NetServerEnrollment.h"
int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    char* end = nullptr;
    const unsigned long pid = std::strtoul(argv[1], &end, 10);
    if (pid == 0 || pid > INT32_MAX || end == nullptr || *end != 0)
    {
        return 2;
    }
    return NetSenderGrantAudit::run(static_cast<int32_t>(pid), NetServerEnrollment::proof);
}

#endif

#endif // NET_SERVICE_GRANT_UNIT_END