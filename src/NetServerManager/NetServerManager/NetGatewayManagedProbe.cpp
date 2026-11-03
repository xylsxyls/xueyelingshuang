#include "NetGatewayManagedProbe.h"
#include "NetServerEnrollment.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <chrono>
#include <cstdio>
#include <cstring>

NetGatewayManagedProbe::NetGatewayManagedProbe() : m_service(nullptr), m_proxy(nullptr), m_held(false), m_internal(false), m_ready(false), m_failed(false)
{

}

void NetGatewayManagedProbe::onServiceReady(uint64_t endpoint)
{
    m_failed = m_failed || m_ready || endpoint == 0;
    m_ready = true;
    const uint8_t missing[] = { 1 };
    if (m_service->sendEvent(1, m_internal ? 0 : 1, 0x7348, missing, sizeof(missing)) != NI_NOT_FOUND)
    {
        m_failed = true;
    }
    std::printf("MANAGED_PROBE_READY internal=%u\n", m_internal ? 1U : 0U);
    std::fflush(stdout);
}

int32_t NetGatewayManagedProbe::reply(const NetInterfaceFrame& request, uint32_t status, const uint8_t* data, uint32_t size)
{
    uint32_t required = 0;
    if (NetInterfaceRpc::encode(NI_RESPONSE, status, 0, data, size, nullptr, 0, required) != NI_BUFFER_TOO_SMALL)
    {
        return NI_PROTOCOL_ERROR;
    }
    std::vector<uint8_t> payload(required);
    if (NetInterfaceRpc::encode(NI_RESPONSE, status, 0, data, size, payload.data(), required, required) != NI_OK)
    {
        return NI_PROTOCOL_ERROR;
    }
    NetInterfaceFrame response = request;
    response.m_kind = NI_RESPONSE;
    response.m_payload = payload.data();
    response.m_payloadSize = required;
    return m_service->send(response);
}

bool NetGatewayManagedProbe::onServiceFrame(const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin)
{
    if (origin.m_endpoint == 0 || origin.m_platform != 3) { m_failed = true; return true; }
    if (frame.m_kind == NI_CANCEL)
    {
        return m_internal || m_proxy->accept(frame, origin);
    }
    NetInterfaceRpcView rpc = {};
    if (!m_internal && frame.m_kind == NI_REQUEST && (frame.m_messageType == 0x7351 || frame.m_messageType == 0x7352) &&
        NetInterfaceRpc::parse(frame, rpc) == NI_OK)
    {
        if (frame.m_messageType == 0x7351 && CSystem::fileExists(CStringManager::Utf8ToUnicode(m_holdFile)))
        {
            if (!m_held) { std::puts("MANAGED_PROBE_HELD"); std::fflush(stdout); m_held = true; }
            return false;
        }
        const int32_t sent = reply(frame, NI_RPC_SUCCESS, rpc.m_data, rpc.m_size);
        if (sent == NI_LIMIT_EXCEEDED) { return false; }
        if (sent != NI_OK) { m_failed = true; }
        std::puts(frame.m_messageType == 0x7351 ? "MANAGED_PROBE_EXECUTED" : "MANAGED_PROBE_HEALTHY");
        std::fflush(stdout);
        return true;
    }
    if (NetInterfaceRpc::parse(frame, rpc) != NI_OK || frame.m_productId != 1 || frame.m_messageType != 0x7345)
    {
        m_failed = true;
        return true;
    }
    if (m_internal)
    {
        if (frame.m_kind != NI_REQUEST || rpc.m_parent == 0) { m_failed = true; return true; }
        if (m_events.size() >= 128) { return false; }
        const int32_t sent = reply(frame, NI_RPC_SUCCESS, rpc.m_data, rpc.m_size);
        if (sent == NI_LIMIT_EXCEEDED) { return false; }
        if (sent != NI_OK) { m_failed = true; }
        else
        {
            m_events.push_back(origin);
            std::printf("MANAGED_PROBE_INTERNAL size=%u\n", rpc.m_size);
            std::fflush(stdout);
        }
        return true;
    }
    const bool accepted = m_proxy->accept(frame, origin);
    if (accepted && frame.m_kind == NI_RESPONSE)
    {
        // 这里只记录代理已接纳响应；独立客户端还必须核对实际返回的根号及正文。
        std::printf("MANAGED_PROBE_ROUNDTRIP size=%u\n", rpc.m_size);
        std::fflush(stdout);
    }
    return accepted;
}

void NetGatewayManagedProbe::onServiceClosed(int32_t reason)
{
    if (m_proxy != nullptr) { m_proxy->reset(); }
    if (reason != 0)
    {
        m_failed = true;
        std::fprintf(stderr, "Managed service closed: internal=%u reason=%d\n", m_internal ? 1U : 0U, reason);
    }
}

void NetGatewayManagedProbe::pumpEvents()
{
    if (!m_internal && m_proxy != nullptr)
    {
        m_proxy->poll();
        if (!m_proxy->valid()) { m_failed = true; return; }
    }
    if (m_events.empty() || m_failed || !m_ready) { return; }
    uint8_t body[24] = { 'N', 'V', 'E', '2', 0, 255, 7, 9 };
    std::memcpy(body + 8, m_events.front().m_device, 16);
    const int32_t result = m_service->sendEvent(m_events.front().m_endpoint, 0, 0x7348, body, sizeof(body));
    if (result == NI_LIMIT_EXCEEDED) { return; }
    if (result != NI_OK) { m_failed = true; return; }
    m_events.pop_front();
    std::puts("MANAGED_PROBE_EVENT");
    std::fflush(stdout);
}

int NetGatewayManagedProbe::run(int32_t pid, const std::string& stopFile, uint32_t duration, bool internal, bool capacity)
{
    if (pid <= 0 || duration == 0 || duration > 60000 || stopFile.empty() ||
        CSystem::fileExists(CStringManager::Utf8ToUnicode(stopFile)))
    {
        return 2;
    }
    NetSenderLocalInbox inbox;
    NetGatewayManagedProbe probe;
    NetSenderService service(inbox);
    NetSenderServiceProxy proxy(service);
    probe.m_proxy = &proxy;
    probe.m_service = &service;
    probe.m_internal = internal;
    probe.m_holdFile = stopFile + ".hold";
    ProcessWork::instance().addProcessReceiveCallback(&inbox);
    if (!ProcessWork::instance().initReceive(16452, 64))
    {
        ProcessWork::instance().uninitReceive(); return 1;
    }
    int result = 1;
    try
    {
        if (service.connectManaged(pid, internal ? 0 : 1, NetServerEnrollment::proof, nullptr, &probe, capacity ? 8 : 32, 1, capacity ? 64 : 0) == NI_OK)
        {
            const auto started = std::chrono::steady_clock::now();
            while (!probe.m_failed && service.poll() &&
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count() < duration &&
                !CSystem::fileExists(CStringManager::Utf8ToUnicode(stopFile)))
            {
                probe.pumpEvents(); CSystem::sleepPrecise(1);
            }
            result = probe.m_ready && !probe.m_failed ? 0 : 1;
        }
    }
    catch (...) { }
    service.shutdown();
    proxy.reset();
    probe.m_proxy = nullptr;
    ProcessWork::instance().uninitReceive();
    return result;
}