#include "NetInterfaceTransportTest.h"
#include <cstring>

int32_t NetInterfaceTransportTestChannel::connect(const char* address, uint32_t port,
    const NetInterfaceTlsOptions& options, NetTransportListener* listener, uint64_t& peer)
{
    (void)address;
    (void)port;
    (void)options;
    (void)listener;
    peer = 0;
    return NI_NOT_SUPPORTED;
}

bool NetInterfaceTransportTestChannel::poll()
{
    return false;
}

int32_t NetInterfaceTransportTestChannel::send(uint64_t peer, const NetInterfaceFrame& frame)
{
    (void)peer;
    (void)frame;
    return NI_NOT_SUPPORTED;
}

void NetInterfaceTransportTestChannel::close(uint64_t peer, int32_t reason)
{
    (void)peer;
    (void)reason;
}

void NetInterfaceTransportTestChannel::shutdown()
{

}

NetInterfaceTransportTestListener::NetInterfaceTransportTestListener() :
m_events(0)
{

}

void NetInterfaceTransportTestListener::onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk)
{
    (void)generation;
    (void)maxRequests;
    (void)maxChunk;
    ++m_events;
}

void NetInterfaceTransportTestListener::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product,
    uint32_t type, uint32_t status, const uint8_t* data, uint32_t size)
{
    (void)generation;
    (void)requestId;
    (void)product;
    (void)type;
    (void)status;
    (void)data;
    (void)size;
    ++m_events;
}

bool NetInterfaceTransportTestListener::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    (void)generation;
    (void)frame;
    ++m_events;
    return false;
}

void NetInterfaceTransportTestListener::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    (void)generation;
    (void)reason;
    (void)detail;
    ++m_events;
}

int32_t NetInterfaceTransportTestListener::entropy(void* context, uint8_t* output, uint32_t size)
{
    const uint32_t mode = *static_cast<const uint32_t*>(context);
    if (mode != 0)
    {
        if (size != 0)
        {
            output[0] = 0x74;
        }
        if (mode == 2)
        {
            throw 1;
        }
        return -1;
    }
    std::memset(output, 0x42, size);
    return 0;
}

void NetInterfaceTransportTestListener::onReady(uint64_t peer)
{
    (void)peer;
    ++m_events;
}

bool NetInterfaceTransportTestListener::onFrame(uint64_t peer, const NetInterfaceFrame& frame)
{
    (void)peer;
    (void)frame;
    ++m_events;
    return false;
}

void NetInterfaceTransportTestListener::onClosed(uint64_t peer, int32_t reason, int32_t detail)
{
    (void)peer;
    (void)reason;
    (void)detail;
    ++m_events;
}