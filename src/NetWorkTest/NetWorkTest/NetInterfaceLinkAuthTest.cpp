#include "NetInterface/NetInterfaceAPI.h"
#include "NetLinkTestCases.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "NetInterfaceTlsTestCases.h"

NetInterfaceLinkAuthTestPair::NetInterfaceLinkAuthTestPair(NetInterfaceLinkTestIdentity& identity, bool server) :
m_link(identity.options(server)),
m_raw(tls(identity, !server), 16384),
m_offset(0)
{

}

NetInterfaceTlsOptions NetInterfaceLinkAuthTestPair::tls(NetInterfaceLinkTestIdentity& identity, bool server)
{
    NetInterfaceTlsOptions options = identity.options(server).m_tls;
    options.authority = NetInterface::trustAuthority(&options.authoritySize);
    options.hostname = server ? nullptr : "node.gateway";
    return options;
}

void NetInterfaceLinkAuthTestPair::word(uint8_t* bytes, uint32_t value)
{
    bytes[0] = static_cast<uint8_t>(value >> 24);
    bytes[1] = static_cast<uint8_t>(value >> 16);
    bytes[2] = static_cast<uint8_t>(value >> 8);
    bytes[3] = static_cast<uint8_t>(value);
}

bool NetInterfaceLinkAuthTestPair::start()
{
    NetInterfaceTunnelRecord hello = {};
    hello.m_kind = NI_TUNNEL_HELLO;
    NetInterfaceTunnelRecord output = {};
    return m_link.accept(hello) == NI_OK && m_link.peekOutput(output) == NI_READY &&
        output.m_kind == NI_TUNNEL_HELLO && m_link.consumeOutput() == NI_OK && step() >= 0;
}

int32_t NetInterfaceLinkAuthTestPair::step()
{
    const int32_t status = m_link.progress(1000);
    if (status < 0)
    {
        return status;
    }
    if (m_raw.progress(1000) < 0)
    {
        return NI_INVALID_STATE;
    }
    NetInterfaceTunnelRecord output = {};
    const int32_t ready = m_link.peekOutput(output);
    if (ready < 0)
    {
        return ready;
    }
    if (ready == NI_READY)
    {
        if (output.m_kind != NI_TUNNEL_CONTROL || m_offset > output.m_size)
        {
            return NI_INVALID_STATE;
        }
        uint32_t consumed = 0;
        if (m_raw.feed(output.m_data + m_offset, output.m_size - m_offset, consumed) < 0)
        {
            return NI_INVALID_STATE;
        }
        m_offset += consumed;
        if (m_offset == output.m_size)
        {
            if (m_link.consumeOutput() != NI_OK)
            {
                return NI_INVALID_STATE;
            }
            m_offset = 0;
        }
    }
    const uint8_t* bytes = nullptr;
    uint32_t size = 0;
    if (m_raw.peekOutput(bytes, size) < 0)
    {
        return NI_INVALID_STATE;
    }
    if (size != 0)
    {
        NetInterfaceTunnelRecord record = {};
        record.m_kind = NI_TUNNEL_CONTROL;
        record.m_data = bytes;
        record.m_size = size < 16384 ? size : 16384;
        const int32_t accepted = m_link.accept(record);
        if (accepted == NI_OK)
        {
            if (m_raw.consumeOutput(record.m_size) != NI_OK)
            {
                return NI_INVALID_STATE;
            }
        }
        else if (accepted != NI_LIMIT_EXCEEDED)
        {
            return accepted;
        }
    }
    return m_link.status();
}

bool NetInterfaceLinkAuthTestPair::receive(uint16_t kind, uint32_t type, std::vector<uint8_t>& body)
{
    for (uint32_t index = 0; index < 10000; ++index)
    {
        if (step() < 0)
        {
            return false;
        }
        NetInterfaceFrame frame = {};
        const int32_t ready = m_raw.peek(frame);
        if (ready < 0)
        {
            return false;
        }
        if (ready == NI_READY)
        {
            if (frame.m_kind != kind || frame.m_messageType != type || frame.m_productId != 0 || frame.m_requestId != 0)
            {
                return false;
            }
            body.assign(frame.m_payload, frame.m_payload + frame.m_payloadSize);
            return m_raw.consume() == NI_OK;
        }
    }
    return false;
}

int32_t NetInterfaceLinkAuthTestPair::settle()
{
    for (uint32_t index = 0; index < 10000; ++index)
    {
        const int32_t status = step();
        if (status < 0 || status == NI_READY)
        {
            return status;
        }
    }
    return NI_NEED_MORE;
}