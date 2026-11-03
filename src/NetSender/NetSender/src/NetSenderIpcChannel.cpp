#include "NetSenderIpcChannel.h"
#include "NetSenderMacro.h"
#include "NetSenderIpcState.h"
#include "CSystem/CSystemAPI.h"

NetSenderIpcChannel::NetSenderIpcChannel(): m_state(new NetSenderIpcState)
{

}

NetSenderIpcChannel::~NetSenderIpcChannel()
{
    delete m_state;
}

int32_t NetSenderIpcChannel::open(int32_t destinationPid, int32_t senderPid, uint32_t payloadLimit)
{
    if (m_state->m_channel || destinationPid <= 0 || senderPid != CSystem::currentProcessPid() ||
        payloadLimit == 0 || payloadLimit > 1048608)
    {
        return NET_SENDER_IPC_INVALID;
    }
    try
    {
        std::unique_ptr<ProcessWorkChannel> channel(new ProcessWorkChannel);
        if (!channel->openSender(destinationPid))
        {
            return NET_SENDER_IPC_UNAVAILABLE;
        }
        if (channel->payloadCapacity() < static_cast<int32_t>(payloadLimit))
        {
            // 只认接收器本代声明值；不能用系统按页分配的额外空间掩盖配置过小。
            return NET_SENDER_IPC_TOO_LARGE;
        }
        m_state->m_channel = std::move(channel);
        m_state->m_payloadLimit = payloadLimit;
        return NET_SENDER_IPC_ACCEPTED;
    }
    catch (...)
    {
        return NET_SENDER_IPC_UNAVAILABLE;
    }
}

int32_t NetSenderIpcChannel::trySend(const uint8_t* data, uint32_t size, int32_t type)
{
    if ((data == nullptr && size != 0) || type < 0)
    {
        return NET_SENDER_IPC_INVALID;
    }
    if (!m_state->m_channel)
    {
        return NET_SENDER_IPC_UNAVAILABLE;
    }
    if (size > m_state->m_payloadLimit)
    {
        return NET_SENDER_IPC_TOO_LARGE;
    }
    // 两套公开状态码前五个值保持一致；v2提交后不重发正文，通知丢失由接收侧有限等待补查。
    return m_state->m_channel->trySend(reinterpret_cast<const char*>(data), static_cast<int32_t>(size), type);
}

int32_t NetSenderIpcChannel::flushNotifications()
{
    return m_state->m_channel && m_state->m_channel->ready() ? NET_SENDER_IPC_ACCEPTED : NET_SENDER_IPC_UNAVAILABLE;
}

uint32_t NetSenderIpcChannel::pendingNotifications() const
{
    return 0;
}

void NetSenderIpcChannel::close()
{
    m_state->clear();
}