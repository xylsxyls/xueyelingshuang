#include "NetSenderCState.h"

NetWorkSenderClientOpaque::NetWorkSenderClientOpaque(const NetInterfaceClientCallbacks& callbacks) :
m_client(nullptr),
m_callbacks(callbacks),
m_callbackDepth(0)
{

}

NetWorkSenderClientOpaque::~NetWorkSenderClientOpaque()
{
    try
    {
        NetSender::instance().destroyClient(m_client);
    }
    catch (...)
    {

    }
}

void NetWorkSenderClientOpaque::onSessionReady(uint64_t generation, uint32_t requests, uint32_t chunk)
{
    ++m_callbackDepth;
    try
    {
        m_callbacks.ready(m_callbacks.context, generation, requests, chunk);
    }
    catch (...)
    {
        // 用户回调和关闭路径均不得越过C边界；即使关闭失败也必须归还回调深度
        try
        {
            NetSender::instance().closeGateway(m_client, NI_INVALID_STATE);
        }
        catch (...)
        {

        }
    }
    --m_callbackDepth;
}

void NetWorkSenderClientOpaque::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product,
    uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    ++m_callbackDepth;
    try
    {
        m_callbacks.result(m_callbacks.context, generation, requestId, product, type, status, bytes, size);
    }
    catch (...)
    {
        // 用户回调和关闭路径均不得越过C边界；即使关闭失败也必须归还回调深度
        try
        {
            NetSender::instance().closeGateway(m_client, NI_INVALID_STATE);
        }
        catch (...)
        {

        }
    }
    --m_callbackDepth;
}

bool NetWorkSenderClientOpaque::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    bool accepted = true;
    ++m_callbackDepth;
    try
    {
        accepted = m_callbacks.frame(m_callbacks.context, generation, &frame) != 0;
    }
    catch (...)
    {
        // 用户回调和关闭路径均不得越过C边界；即使关闭失败也必须归还回调深度
        try
        {
            NetSender::instance().closeGateway(m_client, NI_INVALID_STATE);
        }
        catch (...)
        {

        }
    }
    --m_callbackDepth;
    return accepted;
}

void NetWorkSenderClientOpaque::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    ++m_callbackDepth;
    try
    {
        m_callbacks.closed(m_callbacks.context, generation, reason, detail);
    }
    catch (...)
    {

    }
    --m_callbackDepth;
}