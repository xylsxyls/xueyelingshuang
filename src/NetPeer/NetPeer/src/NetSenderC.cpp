#include "NetSenderC.h"
#include "NetSender.h"
#include "NetSenderCState.h"
#include <memory>

int32_t netSenderClientCreate(const NetInterfaceClientCallbacks* callbacks, NetSenderClientHandle* handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = nullptr;
    if (callbacks == nullptr || callbacks->structSize != sizeof(*callbacks) || callbacks->version != 1 ||
        callbacks->ready == nullptr || callbacks->result == nullptr || callbacks->frame == nullptr || callbacks->closed == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::unique_ptr<NetPeerSenderClientOpaque> value(new NetPeerSenderClientOpaque(*callbacks));
        int32_t result = NetSender::instance().createClient(value->m_client);
        if (result == NI_OK)
        {
            result = NetSender::instance().prepareGateway(value->m_client);
        }
        if (result != NI_OK)
        {
            return result;
        }
        *handle = value.release();
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientConfigureCompression(NetSenderClientHandle handle, const uint32_t* types, uint32_t count)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_callbackDepth.load() != 0)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        return NetSender::instance().configureCompression(handle->m_client, types, count);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientConnectService(NetSenderClientHandle handle, uint32_t product, int32_t gatewayPid,
    const NetInterfaceClientOptions* options)
{
    if (handle == nullptr || options == nullptr || options->structSize != sizeof(*options) || options->version != 1 ||
        gatewayPid != 0 || product == 0 || product > INT32_MAX) { return NI_INVALID_ARGUMENT; }
    if (handle->m_callbackDepth.load() != 0)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        NetSenderGatewayOptions configuration = {};
        configuration.m_network = *options;
        return NetSender::instance().connectServiceGateway(handle->m_client, product, configuration, handle);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientPoll(NetSenderClientHandle handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_callbackDepth.load() != 0)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        return NetSender::instance().pollGateway(handle->m_client);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientBegin(NetSenderClientHandle handle, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t* requestId)
{
    if (requestId != nullptr)
    {
        *requestId = 0;
    }
    if (handle == nullptr || requestId == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        return NetSender::instance().beginRequest(handle->m_client, product, type, timeoutMs, *requestId);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientPost(NetSenderClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        return NetSender::instance().postRequest(handle->m_client, requestId, bytes, size);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientSend(NetSenderClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_callbackDepth.load() != 0)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        return NetSender::instance().sendRequest(handle->m_client, requestId, bytes, size);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientCancel(NetSenderClientHandle handle, uint64_t requestId)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        return NetSender::instance().cancelRequest(handle->m_client, requestId);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientPostData(NetSenderClientHandle handle, const NetInterfaceFrame* frame)
{
    if (handle == nullptr || frame == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        return NetSender::instance().postData(handle->m_client, *frame);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientSendData(NetSenderClientHandle handle, const NetInterfaceFrame* frame)
{
    if (handle == nullptr || frame == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_callbackDepth.load() != 0)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        return NetSender::instance().sendData(handle->m_client, *frame);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientClose(NetSenderClientHandle handle, int32_t reason)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        NetSender::instance().closeGateway(handle->m_client, reason); return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientState(NetSenderClientHandle handle, uint64_t* generation, int32_t* ready)
{
    if (generation != nullptr)
    {
        *generation = 0;
    }
    if (ready != nullptr)
    {
        *ready = 0;
    }
    if (handle == nullptr || generation == nullptr || ready == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        bool value = false;
        const int32_t result = NetSender::instance().gatewayState(handle->m_client, *generation, value);
        *ready = value ? 1 : 0;
        return result;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientDestroy(NetSenderClientHandle handle)
{
    if (handle == nullptr)
    {
        return NI_OK;
    }
    if (handle->m_callbackDepth.load() != 0)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        const int32_t result = NetSender::instance().destroyClient(handle->m_client);
        if (result != NI_OK)
        {
            return result;
        }
        delete handle;
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientConnect(NetSenderClientHandle handle, const NetInterfaceClientOptions* options)
{
    if (handle == nullptr || options == nullptr || options->structSize != sizeof(*options) || options->version != 1)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_callbackDepth.load() != 0)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        NetSenderGatewayOptions configuration = {};
        configuration.m_network = *options;
        return NetSender::instance().connectGateway(handle->m_client, configuration, handle);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientSendBegin(NetSenderClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size, uint64_t* ticket)
{
    if (ticket != nullptr)
    {
        *ticket = 0;
    }
    if (handle == nullptr || ticket == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        return NetSender::instance().beginSendRequest(handle->m_client, requestId, bytes, size, *ticket);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientSendDataBegin(NetSenderClientHandle handle, const NetInterfaceFrame* frame, uint64_t* ticket)
{
    if (ticket != nullptr)
    {
        *ticket = 0;
    }
    if (handle == nullptr || frame == nullptr || ticket == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        return NetSender::instance().beginSendData(handle->m_client, *frame, *ticket);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientSendResult(NetSenderClientHandle handle, uint64_t ticket)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        return NetSender::instance().sendResult(handle->m_client, ticket);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t netSenderClientConfigureGateways(NetSenderClientHandle handle, const char* seeds, const char* cachePath)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_callbackDepth.load() != 0)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        return NetSender::backend().configureGateways(handle->m_client, seeds, cachePath);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}