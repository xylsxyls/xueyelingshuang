#include "NetLinkLocalTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>
#include <cstdio>

/** 实际联调固定截止时间用单调时钟
@return 毫秒
*/
static uint64_t LinkLocalTestNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

NetLinkLocalTestProbe::NetLinkLocalTestProbe() : m_link(nullptr), m_peer(0), m_step(0), m_failed(false), m_ready(false)
{

}

void NetLinkLocalTestProbe::onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity)
{
    m_failed = m_failed || m_ready || peer != m_peer || identity.m_role != NI_LINK_NODE ||
        identity.m_name == nullptr || std::string(identity.m_name).compare(0, 5, "node.") != 0;
    m_ready = true;
}

bool NetLinkLocalTestProbe::onLinkControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    ProtoMessage reply;
    if (peer != m_peer || frame.m_productId != 0 || frame.m_kind != NI_RESPONSE ||
        !reply.from(std::string(reinterpret_cast<const char*>(frame.m_payload), frame.m_payloadSize)))
    {
        m_failed = true;
        return true;
    }
    if (m_step == 1)
    {
        Variant count;
        Variant name;
        Variant address;
        Variant port;
        m_failed = m_failed || frame.m_requestId != UINT64_C(0x8123456789abcdef) ||
            frame.m_messageType != NI_GATEWAY_DISCOVER || reply.getCode() != NI_OK ||
            !reply.getKeyValue(NI_GATEWAY_NODE_COUNT, count) || count.type() != Variant::INT32 || static_cast<int32_t>(count) != 1 ||
            !reply.getKeyValue(NI_GATEWAY_NODE_ID, name, 1) || name.type() != Variant::STRING || name.toString().size() != 37 ||
            !reply.getKeyValue(NI_GATEWAY_ADDRESS, address, 1) || address.type() != Variant::STRING || address.toString() != "127.0.0.1" ||
            !reply.getKeyValue(NI_GATEWAY_PORT, port, 1) || port.type() != Variant::INT32 || static_cast<int32_t>(port) <= 0;
        m_step = 2;
    }
    else if (m_step == 3)
    {
        m_failed = m_failed || frame.m_requestId != UINT64_C(0x8123456789abcdf0) ||
            frame.m_messageType != NI_GATEWAY_OPEN || reply.getCode() != NI_NOT_FOUND;
        m_step = 4;
    }
    else { m_failed = true; }
    return true;
}

bool NetLinkLocalTestProbe::onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    (void)peer;
    (void)record;
    m_failed = true;
    return true;
}

void NetLinkLocalTestProbe::onLinkClosed(uint64_t peer, int32_t reason)
{
    m_failed = m_failed || peer != m_peer || m_step != 4 || reason != 0;
}

int NetLinkLocalTestProbe::childMain(int32_t pid)
{
    NetSenderLocalInbox inbox;
    NetLinkLocalTestProbe probe;
    NetSenderLink link(inbox);
    probe.m_link = &link;
    ProcessWork::instance().addProcessReceiveCallback(&inbox);
    if (!ProcessWork::instance().initReceive(16452, 32))
    {
        ProcessWork::instance().uninitReceive();
        return 51;
    }
    int result = 52;
    try
    {
        NetInterfaceLinkOptions options = {};
        options.m_version = 3;
        options.m_role = NI_LINK_CLIENT;
        options.m_tls.handshakeTimeoutMs = 5000;
        NetInterfaceTlsTestPair::require(link.connect(pid, options, &probe, probe.m_peer) == NI_OK,
            "managed IPC connect failed");
        const uint64_t deadline = LinkLocalTestNow() + 10000;
        while (LinkLocalTestNow() < deadline && !probe.m_failed && probe.m_step != 4)
        {
            link.poll();
            if (probe.m_ready && (probe.m_step == 0 || probe.m_step == 2))
            {
                NetInterfaceFrame request = {};
                request.m_kind = NI_REQUEST;
                request.m_messageType = probe.m_step == 0 ? NI_GATEWAY_DISCOVER : NI_GATEWAY_OPEN;
                request.m_requestId = UINT64_C(0x8123456789abcdef) + (probe.m_step == 0 ? 0 : 1);
                std::string bytes;
                if (probe.m_step == 2)
                {
                    ProtoMessage open;
                    open.setKeyValue(NI_GATEWAY_TARGET_PRODUCT, Variant(static_cast<int32_t>(1)));
                    open.setKeyValue(NI_GATEWAY_CHANNEL, Variant(UINT64_C(1)));
                    NetInterfaceTlsTestPair::require(open.toString(bytes), "encode service-open request failed");
                    request.m_payload = reinterpret_cast<const uint8_t*>(bytes.data());
                    request.m_payloadSize = static_cast<uint32_t>(bytes.size());
                }
                const int32_t sent = link.sendControl(probe.m_peer, request);
                if (sent == NI_OK) { ++probe.m_step; }
                else if (sent != NI_LIMIT_EXCEEDED) { probe.m_failed = true; }
            }
            CSystem::Sleep(1);
        }
        result = probe.m_step == 4 && !probe.m_failed ? 0 : 53;
    }
    catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); }
    catch (...) { }
    link.shutdown();
    ProcessWork::instance().uninitReceive();
    if (result == 0 && !probe.m_failed) { std::puts("MANAGED_LOCAL_GATEWAY_PASS"); return 0; }
    return result == 0 ? 54 : result;
}

#define NET_SERVICE_GRANT_NO_MAIN
#include "NetSender/NetSenderAPI.h"
#include <cstdlib>
#if defined(_WIN32) && defined(_DEBUG)
#include <windows.h>
#include <crtdbg.h>
#endif

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

#include "NetServerManagerTestIncludes.h"

int NetLinkLocalTestProbe::serviceGrantChildMain(int32_t pid)
{
    return NetSenderGrantAudit::run(pid, NetServerEnrollment::proof);
}