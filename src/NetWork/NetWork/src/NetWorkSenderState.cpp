#include "NetWorkMacro.h"
#include "NetWorkSenderState.h"

NetWorkSenderState::NetWorkSenderState() :
m_client(nullptr), m_listener(nullptr), m_lastError(NI_INVALID_STATE)
{

}

NetInterfaceClientCallbacks NetWorkSenderState::callbacks()
{
    NetInterfaceClientCallbacks value = {};
    value.structSize = sizeof(value); value.version = 1; value.context = this;
    value.ready = ready; value.result = result; value.frame = frame; value.closed = closed;
    return value;
}

void NetWorkSenderState::ready(void* context, uint64_t generation, uint32_t requests, uint32_t chunk)
{
    static_cast<NetWorkSenderState*>(context)->m_listener->onSessionReady(generation, requests, chunk);
}

void NetWorkSenderState::result(void* context, uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    static_cast<NetWorkSenderState*>(context)->m_listener->onSessionResult(generation, id, product, type, status, bytes, size);
}

int32_t NetWorkSenderState::frame(void* context, uint64_t generation, const NetInterfaceFrame* value)
{
    return static_cast<NetWorkSenderState*>(context)->m_listener->onSessionFrame(generation, *value) ? 1 : 0;
}

void NetWorkSenderState::closed(void* context, uint64_t generation, int32_t reason, int32_t detail)
{
    static_cast<NetWorkSenderState*>(context)->m_listener->onSessionClosed(generation, reason, detail);
}