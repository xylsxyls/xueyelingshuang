#include <algorithm>
#include "NetInterface/NetInterfaceAPI.h"
#include "NetEndpointRouterTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include <cstring>

NetEndpointProbeSink::NetEndpointProbeSink() :
m_fault(0), m_sent(0), m_closed(0), m_endpoint(0), m_borrowed(nullptr), m_borrowedSize(0), m_router(nullptr)
{
    std::memset(&m_frame, 0, sizeof(m_frame));
    std::memset(&m_origin, 0, sizeof(m_origin));
    std::memset(m_bytes, 0, sizeof(m_bytes));
}

int32_t NetEndpointProbeSink::send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetEndpointOrigin& origin)
{
    m_borrowed = frame.m_payload;
    m_borrowedSize = frame.m_payloadSize;
    if ((m_fault & 1) != 0) { throw 1; }
    if ((m_fault & 4) != 0) { return NI_LIMIT_EXCEEDED; }
    if ((m_fault & 8) != 0 && m_router != nullptr)
    {
        m_router->shutdown();
        m_router->poll();
    }
    ++m_sent;
    m_endpoint = endpoint;
    m_origin = origin;
    m_frame = frame;
    if (frame.m_payloadSize != 0)
    {
        std::memcpy(m_bytes, frame.m_payload, (std::min<uint32_t>)(sizeof(m_bytes), frame.m_payloadSize));
    }
    m_frame.m_payload = m_bytes;
    return NI_OK;
}

void NetEndpointProbeSink::close(uint64_t endpoint)
{
    (void)endpoint;
    ++m_closed;
    if ((m_fault & 2) != 0) { throw 1; }
}

NetEndpointProbeTransport::NetEndpointProbeTransport() :
m_fault(0), m_stopped(0), m_listened(0), m_listener(nullptr)
{

}

int32_t NetEndpointProbeTransport::connect(const char*, uint32_t, const NetInterfaceTlsOptions&, NetTransportListener*, uint64_t& peer)
{
    peer = 0;
    return NI_NOT_SUPPORTED;
}

int32_t NetEndpointProbeTransport::listen(const char*, uint32_t, const NetInterfaceTlsOptions&, NetTransportListener* listener, uint32_t)
{
    ++m_listened;
    m_listener = listener;
    if ((m_fault & 1) != 0) { throw 1; }
    return (m_fault & 32) != 0 ? NI_NOT_SUPPORTED : NI_OK;
}

bool NetEndpointProbeTransport::poll()
{
    if ((m_fault & 8) != 0) { throw 1; }
    return (m_fault & 64) == 0;
}

int32_t NetEndpointProbeTransport::send(uint64_t, const NetInterfaceFrame&)
{
    return NI_OK;
}

void NetEndpointProbeTransport::close(uint64_t, int32_t)
{
    if ((m_fault & 16) != 0) { throw 1; }
}

void NetEndpointProbeTransport::shutdown()
{
    ++m_stopped;
    m_listener = nullptr;
    if ((m_fault & 2) != 0) { throw 1; }
}

uint32_t NetEndpointProbeTransport::localPort() const
{
    if ((m_fault & 4) != 0) { throw 1; }
    return 1025;
}

uint32_t NetEndpointProbeTransport::peerCount() const
{
    return 0;
}