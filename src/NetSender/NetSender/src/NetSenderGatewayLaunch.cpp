#include "NetSenderMacro.h"
#include "NetSenderGatewayLaunch.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "NetInterface/NetInterfaceAPI.h"

std::string NetSenderGatewayLaunch::find(const std::string& businessExecutable)
{
    const std::string base = CSystem::GetName(businessExecutable, 4);
    if (base.empty()) { return ""; }
#ifdef _WIN32
    const std::string name = "NetClientManager1.1.exe";
#else
    const std::string name = "NetClientManager1.1";
#endif
    const std::string sibling = CSystem::joinPath(CSystem::joinPath(base, "../NetClientManager"), name);
    if (CSystem::fileExists(CStringManager::Utf8ToUnicode(sibling)))
    {
        return sibling;
    }
    const std::string flat = CSystem::joinPath(base, name);
    return CSystem::fileExists(CStringManager::Utf8ToUnicode(flat)) ? flat : "";
}

int32_t NetSenderGatewayLaunch::ensure(uint32_t& pid)
{
    pid = 0;
#ifdef _WIN32
    const std::vector<uint32_t> candidates = CSystem::processPid(std::string("NetClientManager1.1.exe"));
    for (uint32_t candidate : candidates)
    {
        if (!CSystem::isProcessLeaseReady("NetClientManager", candidate))
        {
            continue;
        }
        if (pid != 0)
        {
            pid = 0;
            return NI_INVALID_STATE;
        }
        pid = candidate;
    }
    if (pid != 0)
    {
        return NI_OK;
    }
    const std::vector<uint32_t> guards = CSystem::processPid(std::string("NetClientManagerGuard1.1.exe"));
    uint32_t supervisor = 0;
    for (uint32_t candidate : guards)
    {
        if (!CSystem::isProcessLeaseReady("NetClientManagerSupervisor", candidate))
        {
            continue;
        }
        if (supervisor != 0)
        {
            return NI_INVALID_STATE;
        }
        supervisor = candidate;
    }
    // 已有独立守护保留原配置，退避期间不能由业务按默认配置抢先拉起。
    if (supervisor != 0)
    {
        pid = supervisor;
        return NI_OK;
    }
    const std::string executable = find(CSystem::currentExecutablePathUtf8());
    if (executable.empty()) { return NI_NOT_FOUND; }
    uint64_t monitor = 0;
    const uint32_t error = CSystem::startDetachedProcess(executable, std::vector<std::string>(1, "--background"), pid, monitor);
    CSystem::closeProcessMonitor(monitor);
    return error == 0 ? NI_OK : NI_INVALID_STATE;
#else
    return NI_INVALID_STATE;
#endif
}