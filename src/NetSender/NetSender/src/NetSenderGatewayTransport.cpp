#include "NetSenderMacro.h"
#include "NetSenderGatewayTransport.h"
#include "CSystem/CSystemAPI.h"
#include "NetSenderGatewayLaunch.h"

NetSenderGatewayTransportFactory::NetSenderGatewayTransportFactory(NetSenderLocalInbox& inbox, int32_t pid) : m_inbox(&inbox), m_pid(pid)
{

}

NetGatewayTransport* NetSenderGatewayTransportFactory::create()
{
    // 每次新代次重新发现PID，本机网关重启后不复用旧进程地址。
#ifdef _WIN32
    const char* executable = "NetClientManager1.1.exe";
#else
    const char* executable = "NetClientManager1.1";
#endif
    const std::vector<uint32_t> candidates = m_pid == 0 ? CSystem::processPid(std::string(executable)) :
        std::vector<uint32_t>(1, static_cast<uint32_t>(m_pid));
    int32_t pid = 0;
    for (uint32_t candidate : candidates)
    {
        if (!CSystem::isProcessLeaseReady("NetClientManager", candidate))
        {
            continue;
        }
        if (pid != 0) { pid = -1; break; }
        pid = static_cast<int32_t>(candidate);
    }
#ifdef _WIN32
    if (m_pid == 0 && pid == 0)
    {
        uint32_t started = 0;
        // 接纳启动不等于ready；本代仍报告可恢复NOT_FOUND，由既有退避重发现。
        NetSenderGatewayLaunch::ensure(started);
    }
#endif
    return new NetSenderGatewayTransport(*m_inbox, pid);
}

NetSenderGatewayTransport::NetSenderGatewayTransport(NetSenderLocalInbox& inbox, int32_t pid) : m_link(inbox), m_pid(pid)
{

}

NetSenderGatewayTransport::~NetSenderGatewayTransport()
{
    shutdown();
}

int32_t NetSenderGatewayTransport::connect(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
    NetLinkTransportListener* listener, uint64_t& peer)
{
    (void)address;
    (void)port;
    peer = 0;
    // 网关尚未启动属于可恢复的目标不存在，不能误判为永久配置错误。
    if (m_pid < 0) { return NI_INVALID_ARGUMENT; }
    if (m_pid == 0 || !CSystem::isProcessLeaseReady("NetClientManager", static_cast<uint32_t>(m_pid)))
    {
        return NI_NOT_FOUND;
    }
    return m_link.connect(m_pid, options, listener, peer);
}

bool NetSenderGatewayTransport::poll()
{
    return m_link.poll();
}

int32_t NetSenderGatewayTransport::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    return m_link.sendControl(peer, frame);
}

int32_t NetSenderGatewayTransport::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    return m_link.sendRecord(peer, record);
}

void NetSenderGatewayTransport::shutdown()
{
    m_link.shutdown();
}

int32_t NetSenderGatewayTransport::outputState(uint64_t peer) const
{
    return m_link.outputState(peer);
}