#include "NetSenderMacro.h"
#include "NetSenderIpc.h"
#include "NetSenderGatewayState.h"
#include "NetSenderLocalHub.h"
#include "NetSenderDesktopBackend.h"
#include "NetSenderGatewayLaunch.h"
#include <cstring>
#include "CSystem/CSystemAPI.h"

NetSenderIpc::NetSenderIpc()
{

}

NetSenderIpc::~NetSenderIpc()
{

}

NetSenderIpc& NetSenderIpc::instance()
{
    static NetSenderIpc s_instance;
    return s_instance;
}

int32_t NetSenderIpc::createClient(NetSenderGatewayState*& client)
{
    if (client != nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        client = new NetSenderGatewayState();
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetSenderIpc::prepareGateway(NetSenderGatewayState* client)
{
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (client->m_dispatch.executing())
    {
        client->m_lastError = NI_INVALID_STATE;
        return NI_INVALID_STATE;
    }
    try
    {
        if (!client->m_receiverLease)
        {
            const int32_t acquired = NetSenderLocalHub::instance().acquire();
            if (acquired != NI_OK)
            {
                client->m_lastError = acquired; return acquired;
            }
            client->m_receiverLease = true;
        }
        if (!client->m_registered)
        {
            const int32_t attached = NetSenderLocalHub::instance().attach(&client->m_inbox);
            if (attached != NI_OK)
            {
                client->m_lastError = attached; return attached;
            }
            client->m_registered = true;
        }
        client->m_lastError = NI_OK;
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetSenderIpc::connectGateway(NetSenderGatewayState* client, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    if (client == nullptr || !client->m_receiverLease || !client->m_registered)
    {
        return NI_INVALID_STATE;
    }
    if (options.m_network.structSize != sizeof(NetWorkClientOptions) || options.m_network.version != 1 || listener == nullptr)
    {
        client->m_lastError = NI_INVALID_ARGUMENT;
        return client->m_lastError;
    }
    // 先核对活跃/回调状态，不能提前销毁会话仍借用的旧通道。
    if (!client->m_dispatch.quiescent())
    {
        client->m_lastError = NI_INVALID_STATE; return NI_INVALID_STATE;
    }
    const int32_t detached = client->m_session.bindChannel(nullptr);
    if (detached != NI_OK)
    {
        client->m_lastError = detached;
        return detached;
    }
    try
    {
        NetInterfaceHandshakeOptions handshake = {};
        client->m_serviceFactory.reset();
        handshake.m_platform = options.m_network.platform;
        handshake.m_capabilities = options.m_network.capabilities;
        handshake.m_maxRequests = options.m_network.maxRequests;
        handshake.m_maxChunk = options.m_network.maxChunk;
        handshake.m_timeoutMs = options.m_network.handshakeTimeoutMs;
        std::memcpy(handshake.m_device, options.m_network.device, sizeof(handshake.m_device));
        if (options.m_localGatewayPid != 0 || handshake.m_platform < 1 || handshake.m_platform > 3)
        {
            client->m_lastError = NI_INVALID_ARGUMENT;
            return NI_INVALID_ARGUMENT;
        }
        int32_t pid = client->m_backendPid;
        if (pid == 0)
        {
            uint32_t launched = 0;
            const int32_t ensured = NetSenderGatewayLaunch::ensure(launched);
            if (ensured != NI_OK || launched == 0 || launched > INT32_MAX)
            {
                client->m_lastError = ensured == NI_OK ? NI_NOT_FOUND : ensured;
                return client->m_lastError;
            }
            pid = static_cast<int32_t>(launched);
        }
        client->m_channel.reset(new NetSenderClientChannel(client->m_inbox, pid));
        const int32_t bound = client->m_session.bindChannel(client->m_channel.get());
        if (bound != NI_OK)
        {
            client->m_lastError = bound;
            return bound;
        }
        client->m_lastError = client->m_dispatch.prepare(listener);
        if (client->m_lastError != NI_OK)
        {
            return client->m_lastError;
        }
        client->m_lastError = client->m_session.connect("local", 1, NetSenderGatewayState::clientTls(options.m_network.tls), handshake, client->m_dispatch.listener());
        if (client->m_lastError == NI_OK)
        {
            client->m_lastError = client->m_dispatch.started(client->m_session.generation(), 0);
        }
        return client->m_lastError;
    }
    catch (...)
    {
        client->m_lastError = NI_NO_MEMORY;
        return NI_NO_MEMORY;
    }
}

int32_t NetSenderIpc::connectServiceGateway(NetSenderGatewayState* client, uint32_t product, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    if (client == nullptr || !client->m_receiverLease || !client->m_registered)
    {
        return NI_INVALID_STATE;
    }
    if (product == 0 || product > INT32_MAX || listener == nullptr || options.m_network.structSize != sizeof(NetWorkClientOptions) || options.m_network.version != 1)
    {
        client->m_lastError = NI_INVALID_ARGUMENT;
        return NI_INVALID_ARGUMENT;
    }
    if (!client->m_dispatch.quiescent())
    {
        client->m_lastError = NI_INVALID_STATE; return NI_INVALID_STATE;
    }
    const int32_t detached = client->m_session.bindChannel(nullptr);
    if (detached != NI_OK)
    {
        client->m_lastError = detached; return detached;
    }
    try
    {
        NetInterfaceHandshakeOptions handshake = {};
        handshake.m_platform = options.m_network.platform;
        handshake.m_capabilities = options.m_network.capabilities;
        handshake.m_maxRequests = options.m_network.maxRequests;
        handshake.m_maxChunk = options.m_network.maxChunk;
        handshake.m_timeoutMs = options.m_network.handshakeTimeoutMs;
        std::memcpy(handshake.m_device, options.m_network.device, sizeof(handshake.m_device));
        client->m_serviceProduct = product;
        if (options.m_localGatewayPid != 0 || handshake.m_platform < 1 || handshake.m_platform > 3)
        {
            client->m_lastError = NI_INVALID_ARGUMENT;
            return NI_INVALID_ARGUMENT;
        }
        client->m_serviceFactory.reset(new NetSenderGatewayTransportFactory(client->m_inbox, client->m_backendPid));
        NetGatewayTransportFactory* factory = client->m_serviceFactory.get();
        const char* address = "local";
        const uint32_t port = 1;
        client->m_lastError = client->m_dispatch.prepare(listener);
        if (client->m_lastError != NI_OK)
        {
            return client->m_lastError;
        }
        client->m_lastError = client->m_session.connectService(product, address, port, NetSenderGatewayState::clientTls(options.m_network.tls), handshake, client->m_dispatch.listener(), factory);
        if (client->m_lastError == NI_OK)
        {
            client->m_lastError = client->m_dispatch.started(client->m_session.generation(), product);
        }
        return client->m_lastError;
    }
    catch (...)
    {
        client->m_lastError = NI_NO_MEMORY; return NI_NO_MEMORY;
    }
}

int32_t NetSenderIpc::configureCompression(NetSenderGatewayState* client, const uint32_t* types, uint32_t count)
{
    if (client == nullptr || !client->m_receiverLease || !client->m_registered)
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = client->m_session.configureCompression(types, count);
    return client->m_lastError;
}

int32_t NetSenderIpc::pollGateway(NetSenderGatewayState* client)
{
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (client->m_dispatch.executing())
    {
        return NI_INVALID_STATE;
    }
    return client->m_dispatch.poll() ? NI_READY : NI_OK;
}

int32_t NetSenderIpc::beginRequest(NetSenderGatewayState* client, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& requestId)
{
    requestId = 0;
    if (client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    if (client->m_session.serviceMode() && client->m_serviceProduct != 0 && product != client->m_serviceProduct)
    {
        client->m_lastError = NI_INVALID_ARGUMENT;
        return NI_INVALID_ARGUMENT;
    }
    client->m_lastError = client->m_dispatch.submission().begin(product, type, timeoutMs, requestId);
    return client->m_lastError;
}

int32_t NetSenderIpc::postRequest(NetSenderGatewayState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    if (client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = client->m_dispatch.submission().send(requestId, bytes, size);
    return client->m_lastError;
}

int32_t NetSenderIpc::cancelRequest(NetSenderGatewayState* client, uint64_t requestId)
{
    if (client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = client->m_dispatch.submission().cancel(requestId);
    return client->m_lastError;
}

int32_t NetSenderIpc::postData(NetSenderGatewayState* client, const NetInterfaceFrame& frame)
{
    if (client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = client->m_dispatch.submission().data(frame);
    return client->m_lastError;
}

void NetSenderIpc::closeGateway(NetSenderGatewayState* client, int32_t reason)
{
    if (client != nullptr)
    {
        client->m_dispatch.submission().close(reason);
    }
}

void NetSenderIpc::uninitGateway(NetSenderGatewayState* client)
{
    if (client != nullptr)
    {
        client->m_dispatch.submission().close(0);
    }
}

int32_t NetSenderIpc::destroyClient(NetSenderGatewayState*& client)
{
    if (client != nullptr && !client->m_dispatch.quiescent())
    {
        client->m_lastError = NI_INVALID_STATE;
        return NI_INVALID_STATE;
    }
    if (client != nullptr && client->m_registered)
    {
        NetSenderLocalHub::instance().detach(&client->m_inbox);
    }
    if (client != nullptr && client->m_receiverLease)
    {
        NetSenderLocalHub::instance().release();
    }
    delete client;
    client = nullptr;
    return NI_OK;
}

bool NetSenderIpc::gatewayReady(NetSenderGatewayState* client) const
{
    uint64_t generation = 0; bool ready = false;
    if (client != nullptr)
    {
        client->m_dispatch.submission().state(generation, ready);
    }
    return ready;
}

uint64_t NetSenderIpc::gatewayGeneration(NetSenderGatewayState* client) const
{
    uint64_t generation = 0; bool ready = false;
    if (client != nullptr)
    {
        client->m_dispatch.submission().state(generation, ready);
    }
    return generation;
}

int32_t NetSenderIpc::lastGatewayError(NetSenderGatewayState* client) const
{
    return client != nullptr ? client->m_lastError : NI_INVALID_STATE;
}

int32_t NetSenderIpc::sendRequest(NetSenderGatewayState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    if (client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = client->m_dispatch.sendRequest(requestId, bytes, size);
    return client->m_lastError;
}

int32_t NetSenderIpc::sendData(NetSenderGatewayState* client, const NetInterfaceFrame& frame)
{
    if (client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = client->m_dispatch.sendData(frame);
    return client->m_lastError;
}

int32_t NetSenderIpc::gatewayState(NetSenderGatewayState* client, uint64_t& generation, bool& ready)
{
    generation = 0;
    ready = false;
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    client->m_dispatch.submission().state(generation, ready);
    return NI_OK;
}

int32_t NetSenderIpc::beginSendRequest(NetSenderGatewayState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size, uint64_t& ticket)
{
    ticket = 0;
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (client->m_dispatch.executing())
    {
        return NI_INVALID_STATE;
    }
    try
    {
        const int32_t capability = client->m_session.outputState();
        if (capability < 0)
        {
            return capability;
        }
        return client->m_dispatch.submission().sendSynchronous(requestId, bytes, size, ticket);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetSenderIpc::beginSendData(NetSenderGatewayState* client, const NetInterfaceFrame& frame, uint64_t& ticket)
{
    ticket = 0;
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (client->m_dispatch.executing())
    {
        return NI_INVALID_STATE;
    }
    try
    {
        const int32_t capability = client->m_session.outputState();
        if (capability < 0)
        {
            return capability;
        }
        return client->m_dispatch.submission().dataSynchronous(frame, ticket);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetSenderIpc::sendResult(NetSenderGatewayState* client, uint64_t ticket)
{
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        return client->m_dispatch.submission().synchronousResult(ticket, true);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

NetFileProtocol& NetSenderIpc::fileProtocol()
{
    static NetFileProtocol s_protocol;
    return s_protocol;
}

uint32_t NetSenderIpc::pointerBytes()
{
    return static_cast<uint32_t>(sizeof(void*));
}

int32_t NetSenderIpc::randomBytes(uint8_t* output, uint32_t size)
{
    if ((output == nullptr && size != 0) || size > 8192)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (size == 0)
    {
        return NI_OK;
    }
    try
    {
        if (CSystem::secureRandomBytes(output, size))
        {
            return NI_OK;
        }
    }
    catch (...)
    {
        // 熵源失败时统一清理有效输出，不能将未完成数据用于密码学用途
    }
    volatile uint8_t* target = output;
    for (uint32_t index = 0; index < size; ++index)
    {
        target[index] = 0;
    }
    return NI_INVALID_STATE;
}

NetSenderDesktopBackend& NetSenderIpc::backend()
{
    static NetSenderDesktopBackend s_backend;
    return s_backend;
}