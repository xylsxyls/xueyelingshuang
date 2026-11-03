#include "NetSenderMacro.h"
#include "NetSenderLocalHub.h"
#include "NetSenderLocalPacket.h"
#include <algorithm>

NetSenderLocalHub::NetSenderLocalHub() :
m_phase(NET_LOCAL_HUB_IDLE),
m_clients(0)
{

}

NetSenderLocalHub::~NetSenderLocalHub()
{

}

NetSenderLocalHub& NetSenderLocalHub::instance()
{
    static NetSenderLocalHub s_hub;
    return s_hub;
}

int32_t NetSenderLocalHub::attach(NetSenderLocalInbox* inbox)
{
    if (inbox == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if ((m_phase != NET_LOCAL_HUB_MANAGED && m_phase != NET_LOCAL_HUB_EXTERNAL) || m_clients == 0)
    {
        return NI_INVALID_STATE;
    }
    if (std::find(m_boxes.begin(), m_boxes.end(), inbox) != m_boxes.end())
    {
        return NI_OK;
    }
    if (m_boxes.size() >= 64)
    {
        return NI_LIMIT_EXCEEDED;
    }
    try
    {
        m_boxes.push_back(inbox);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

void NetSenderLocalHub::detach(NetSenderLocalInbox* inbox)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto found = std::find(m_boxes.begin(), m_boxes.end(), inbox);
        if (found != m_boxes.end())
        {
            m_boxes.erase(found);
        }
    }
    stopUnused();
}

int32_t NetSenderLocalHub::prepare(bool external)
{
    bool registered = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (external && m_phase == NET_LOCAL_HUB_EXTERNAL)
        {
            return NI_OK;
        }
        if (!external && (m_phase == NET_LOCAL_HUB_MANAGED || m_phase == NET_LOCAL_HUB_EXTERNAL))
        {
            if (m_clients >= 64)
            {
                return NI_LIMIT_EXCEEDED;
            }
            // 管理模式由本状态排除init/uninit；外部模式仍须宿主按原规范串行管理。
            if (ProcessWork::instance().receiveBufferCount() == 0)
            {
                return NI_INVALID_STATE;
            }
            if (ProcessWork::instance().receivePayloadCapacity() < kNetSenderLocalReceiveBytes - 12)
            {
                return NI_BUFFER_TOO_SMALL;
            }
            ++m_clients;
            return NI_OK;
        }
        if ((m_phase != NET_LOCAL_HUB_IDLE && m_phase != NET_LOCAL_HUB_REGISTERED) || m_clients != 0 || !m_boxes.empty())
        {
            return NI_INVALID_STATE;
        }
        registered = m_phase == NET_LOCAL_HUB_REGISTERED;
        m_phase = NET_LOCAL_HUB_STARTING;
    }
    int32_t result = NI_INVALID_STATE;
    try
    {
        // 不接管已工作的外部接收器；登记只能发生在尚未启动时。
        if (ProcessWork::instance().receiveBufferCount() == 0)
        {
            if (!registered)
            {
                ProcessWork::instance().addProcessReceiveCallback(this);
                registered = true;
            }
            result = external || ProcessWork::instance().initReceive(kNetSenderLocalReceiveBytes, 64) ? NI_OK : NI_INVALID_STATE;
        }
    }
    catch (...)
    {
        result = NI_NO_MEMORY;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (result == NI_OK)
        {
            m_phase = external ? NET_LOCAL_HUB_EXTERNAL : NET_LOCAL_HUB_MANAGED;
            m_clients = external ? 0 : 1;
        }
        else
        {
            // init失败保留回调；再次尝试不能重复登记或uninit掉宿主原Post模块。
            m_phase = registered ? NET_LOCAL_HUB_REGISTERED : NET_LOCAL_HUB_IDLE;
        }
    }
    return result;
}

int32_t NetSenderLocalHub::acquire()
{
    return prepare(false);
}

int32_t NetSenderLocalHub::prepareExternal()
{
    return prepare(true);
}

int32_t NetSenderLocalHub::externalStopped()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_phase != NET_LOCAL_HUB_EXTERNAL || m_clients != 0 || !m_boxes.empty() ||
        ProcessWork::instance().receiveBufferCount() != 0)
    {
        return NI_INVALID_STATE;
    }
    m_phase = NET_LOCAL_HUB_IDLE;
    return NI_OK;
}

void NetSenderLocalHub::release()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_clients != 0)
        {
            --m_clients;
        }
    }
    stopUnused();
}

void NetSenderLocalHub::stopUnused()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_phase != NET_LOCAL_HUB_MANAGED || m_clients != 0 || !m_boxes.empty())
        {
            return;
        }
        m_phase = NET_LOCAL_HUB_STOPPING;
    }
    try
    {
        ProcessWork::instance().uninitReceive();
    }
    catch (...)
    {
        // 停止未被证明完成时，不能让新对象复用未知状态；稳定Hub本身仍存活。
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_phase = NET_LOCAL_HUB_IDLE;
    }
}

void NetSenderLocalHub::receive(int32_t sendPid, const char* bytes, int32_t size, MessageType type)
{
    if (static_cast<int32_t>(type) != NET_SENDER_LOCAL_MESSAGE || sendPid <= 0 || bytes == nullptr || size < 56 || size > 16440)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    for (size_t index = 0; index < m_boxes.size(); ++index)
    {
        try
        {
            m_boxes[index]->receive(sendPid, bytes, size, type);
        }
        catch (...)
        {
            // 单个收件箱的异常不能阻断其他借用箱；该通道仍受原固定超时约束。
        }
    }
}

// NET_SENDER_HUB_LIFECYCLE_UNIT_BEGIN
// #include <cstdio>
//
// int main()
// {
//     uint32_t checks = 0;
// #define HUB_TEST(value, reason) { ++checks; if (!(value)) { std::printf("HUB_FAIL %s\n", reason); throw false; } }
//     try
//     {
//         ProcessWork& work = ProcessWork::instance();
//         NetSenderLocalHub& hub = NetSenderLocalHub::instance();
//         NetSenderLocalInbox first;
//         NetSenderLocalInbox second;
//         uint32_t leases = 0;
//         bool external = false;
//         HUB_TEST(work.receiveBufferCount() == 0, "hub test requires an idle test receiver");
//         try
//         {
//             // 只attach曾经会偷偷登记Hub回调，detach又忘记登记，后续重试造成重复回调。
//             HUB_TEST(hub.attach(&first) == NI_INVALID_STATE && hub.attach(nullptr) == NI_INVALID_ARGUMENT,
//                 "unowned attach registered a ProcessWork callback");
//             HUB_TEST(hub.prepareExternal() == NI_OK, "hub external preparation");
//             external = true;
//             HUB_TEST(hub.prepareExternal() == NI_OK && hub.acquire() == NI_INVALID_STATE && hub.attach(&first) == NI_INVALID_STATE,
//                 "hub lease accepted before receiver readiness");
//             HUB_TEST(work.initReceive(kNetSenderLocalReceiveBytes, 4), "hub external receiver startup");
//             HUB_TEST(hub.acquire() == NI_OK, "hub first external lease");
//             ++leases;
//             HUB_TEST(hub.acquire() == NI_OK, "hub second external lease");
//             ++leases;
//             HUB_TEST(hub.attach(&first) == NI_OK && hub.attach(&first) == NI_OK && hub.attach(&second) == NI_OK,
//                 "hub repeated attach or shared targets");
//             hub.release();
//             --leases;
//             hub.detach(&first);
//             HUB_TEST(work.receiveBufferCount() == 4 && hub.externalStopped() == NI_INVALID_STATE,
//                 "hub released another external client");
//             hub.release();
//             --leases;
//             hub.detach(&second);
//             HUB_TEST(work.receiveBufferCount() == 4 && hub.externalStopped() == NI_INVALID_STATE,
//                 "hub stopped a host-owned receiver");
//             work.uninitReceive();
//             HUB_TEST(hub.externalStopped() == NI_OK, "hub external registration did not return to idle");
//             external = false;
//             HUB_TEST(hub.attach(&first) == NI_INVALID_STATE, "hub retained a stale registration");
//             HUB_TEST(hub.acquire() == NI_OK, "hub managed startup after external shutdown");
//             ++leases;
//             HUB_TEST(hub.attach(&first) == NI_OK && work.receivePayloadCapacity() == 16440, "hub managed receiver capacity");
//             hub.release();
//             --leases;
//             HUB_TEST(work.receiveBufferCount() == 64, "hub stopped while a borrowed inbox was still attached");
//             hub.detach(&first);
//             HUB_TEST(work.receiveBufferCount() == 0, "last detach did not stop managed receiver");
//             hub.release();
//             HUB_TEST(work.receiveBufferCount() == 0, "extra release restarted receiver");
//             for (uint32_t index = 0; index < 64; ++index)
//             {
//                 HUB_TEST(hub.acquire() == NI_OK, "hub lease capacity below 64");
//                 ++leases;
//             }
//             HUB_TEST(hub.acquire() == NI_LIMIT_EXCEEDED, "hub unbounded lease count");
//             while (leases != 0)
//             {
//                 hub.release();
//                 --leases;
//             }
//             HUB_TEST(work.receiveBufferCount() == 0, "hub final lease leaked receiver tasks");
//         }
//         catch (...)
//         {
//             hub.detach(&first);
//             hub.detach(&second);
//             while (leases != 0)
//             {
//                 hub.release();
//                 --leases;
//             }
//             work.uninitReceive();
//             if (external)
//             {
//                 hub.externalStopped();
//             }
//             throw;
//         }
//     }
//     catch (...)
//     {
//         return 1;
//     }
//     std::printf("NetSender Hub ownership checks=%u PASS\n", checks);
//     return 0;
// #undef HUB_TEST
// }
// NET_SENDER_HUB_LIFECYCLE_UNIT_END