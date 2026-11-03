#include "NetInterface/NetInterfaceAPI.h"
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

NetSenderDispatchTestChannel::NetSenderDispatchTestChannel(const NetInterfaceHandshakeOptions& options) :
m_reply(),
m_handshake(new NetInterfaceHandshake(options, NetSenderSubmission::now())),
m_listener(nullptr),
m_announced(false),
m_closing(false),
m_reason(0)
{

}

NetSenderDispatchTestChannel::~NetSenderDispatchTestChannel()
{
    shutdown();
}

int32_t NetSenderDispatchTestChannel::connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
    NetTransportListener* listener, uint64_t& peer)
{
    (void)address;
    (void)port;
    (void)options;
    peer = 0;
    if (m_listener != nullptr || listener == nullptr)
    {
        return NI_INVALID_STATE;
    }
    m_listener = listener;
    peer = 1;
    return NI_OK;
}

bool NetSenderDispatchTestChannel::poll()
{
    if (m_closing)
    {
        shutdown();
        return false;
    }
    if (m_listener == nullptr)
    {
        return false;
    }
    if (!m_announced)
    {
        m_announced = true;
        m_listener->onReady(1);
    }
    NetInterfaceFrame frame = {};
    if (!m_closing && m_handshake->peekOutput(frame) == NI_READY && m_listener->onFrame(1, frame))
    {
        m_handshake->consumeOutput();
    }
    if (!m_closing && m_reply.m_kind != 0 && m_listener->onFrame(1, m_reply))
    {
        m_reply = NetInterfaceFrame();
        m_payload.clear();
    }
    return true;
}

int32_t NetSenderDispatchTestChannel::send(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != 1 || m_listener == nullptr || m_closing)
    {
        return NI_INVALID_STATE;
    }
    if (frame.m_kind == NI_REQUEST)
    {
        if (m_reply.m_kind != 0) { return NI_LIMIT_EXCEEDED; }
        NetInterfaceRpcView view = {};
        if (NetInterfaceRpc::parse(frame, view) != NI_OK)
        {
            return NI_PROTOCOL_ERROR;
        }
        m_requests.push_back(frame.m_requestId);
        m_bytes.push_back(view.m_size == 0 ? 0 : view.m_data[0]);
        m_payload.resize(view.m_size + 16);
        uint32_t required = 0;
        const int32_t encoded = NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, view.m_data, view.m_size, m_payload.data(), static_cast<uint32_t>(m_payload.size()), required);
        if (encoded != NI_OK) { return encoded; }
        m_reply = frame;
        m_reply.m_kind = NI_RESPONSE;
        m_reply.m_payload = m_payload.data();
        m_reply.m_payloadSize = required;
        return NI_OK;
    }
    if (frame.m_kind == NI_CANCEL) { return NI_OK; }
    const int32_t accepted = m_handshake->accept(frame, NetSenderSubmission::now());
    return accepted < 0 ? accepted : NI_OK;
}

void NetSenderDispatchTestChannel::close(uint64_t peer, int32_t reason)
{
    if (peer == 1 && !m_closing)
    {
        m_closing = true;
        m_reason = reason;
    }
}

int32_t NetSenderDispatchTestChannel::outputState(uint64_t peer) const
{
    return peer == 1 && m_listener != nullptr && !m_closing ? NI_OK : NI_INVALID_STATE;
}

void NetSenderDispatchTestChannel::shutdown()
{
    m_closing = true;
    NetTransportListener* listener = m_listener;
    m_listener = nullptr;
    if (listener != nullptr)
    {
        listener->onClosed(1, m_reason, 0);
    }
}