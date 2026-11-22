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
    const int32_t result = m_state->m_channel->trySend(reinterpret_cast<const char*>(data), static_cast<int32_t>(size), type);
    // 显式翻译依赖状态，避免依赖新增错误码后被当成已提交或不可恢复的连接故障。
    switch (result)
    {
    case PROCESS_WORK_SENT: return NET_SENDER_IPC_ACCEPTED;
    case PROCESS_WORK_BUSY: return NET_SENDER_IPC_BUSY;
    case PROCESS_WORK_INVALID: return NET_SENDER_IPC_INVALID;
    case PROCESS_WORK_TOO_LARGE: return NET_SENDER_IPC_TOO_LARGE;
    case PROCESS_WORK_NO_MEMORY: return NET_SENDER_IPC_NO_MEMORY;
    case PROCESS_WORK_CAPACITY: return NET_SENDER_IPC_CAPACITY;
    case PROCESS_WORK_RESOURCE: return NET_SENDER_IPC_RESOURCE;
    default: return NET_SENDER_IPC_UNAVAILABLE;
    }
}

bool NetSenderIpcChannel::retryable(int32_t result)
{
    return result == NET_SENDER_IPC_BUSY || result == NET_SENDER_IPC_CAPACITY || result == NET_SENDER_IPC_RESOURCE;
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