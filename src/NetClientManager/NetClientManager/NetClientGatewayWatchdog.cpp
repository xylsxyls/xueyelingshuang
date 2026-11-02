#include "NetClientGatewayWatchdog.h"
#include "NetClientGatewayLifetime.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <chrono>
#include <memory>
#include <cstdio>

bool NetClientGatewayWatchdog::start(const std::vector<std::string>& arguments)
{
#ifdef _WIN32
    const uint32_t parent = CSystem::currentProcessPid();
    const std::string slot = "NetClientManagerSupervisor";
    std::vector<std::string> options(arguments);
    options.insert(options.begin(), "--watch-parent=" + std::to_string(parent));
    uint32_t pid = 0;
    uint64_t monitor = 0;
    const std::string executable = CSystem::joinPath(CSystem::GetName(CSystem::currentExecutablePathUtf8(), 4),
        "NetClientManagerGuard1.1.exe");
    if (!CSystem::fileExists(CStringManager::Utf8ToUnicode(executable)))
    {
        std::fputs("CLIENT_GATEWAY_WATCHDOG_FAILED: missing NetClientManagerGuard1.1.exe beside gateway\n", stderr);
        return false;
    }
    if (CSystem::startDetachedProcess(executable, options, pid, monitor) != 0)
    {
        return false;
    }
    const std::shared_ptr<void> guard(nullptr, [&monitor](void*) { CSystem::closeProcessMonitor(monitor); });
    const auto begin = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - begin < std::chrono::seconds(3))
    {
        if (CSystem::isProcessLeaseReady(slot, pid))
        {
            std::printf("CLIENT_GATEWAY_WATCHDOG pid=%u\n", pid);
            std::fflush(stdout);
            return true;
        }
        int32_t code = 0;
        if (CSystem::pollProcessMonitor(monitor, code) != 0)
        {
            break;
        }
        CSystem::Sleep(10);
    }
    return false;
#else
    (void)arguments;
    return true;
#endif
}

bool NetClientGatewayWatchdog::waitForRecovery()
{
#ifdef _WIN32
    const std::vector<uint32_t> candidates = CSystem::processPid(std::string("NetClientManagerGuard1.1.exe"));
    for (uint32_t candidate : candidates)
    {
        if (!CSystem::isProcessLeaseReady("NetClientManagerSupervisor", candidate))
        {
            continue;
        }
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        // 正常退出的旧网关已释放租约，但守护尚未读取其退出码。
        // 等待实际ready或守护释放，避免把这个交接窗口误判为持续崩溃恢复。
        while (CSystem::isProcessLeaseReady("NetClientManagerSupervisor", candidate))
        {
            const std::vector<uint32_t> gateways = CSystem::processPid(std::string("NetClientManager1.1.exe"));
            for (uint32_t gateway : gateways)
            {
                if (CSystem::isProcessLeaseReady("NetClientManager", gateway))
                {
                    return true;
                }
            }
            if (std::chrono::steady_clock::now() >= deadline)
            {
                return true;
            }
            CSystem::Sleep(10);
        }
    }
#endif
    return false;
}

int NetClientGatewayWatchdog::run(uint32_t parent, const std::string& executable, const std::vector<std::string>& arguments)
{
#ifdef _WIN32
    const std::string slot = "NetClientManagerSupervisor";
    std::vector<uint64_t> lease;
    uint64_t monitor = CSystem::openProcessMonitor(parent);
    const std::shared_ptr<void> guard(nullptr, [&monitor, &lease](void*) {
        CSystem::closeProcessMonitor(monitor);
        CSystem::releaseProcessLease(lease);
    });
    if (monitor == 0) { return 1; }
    NetClientGatewayLifetime lifetime;
    if (!lifetime.init([]() {})) { return 1; }
    const auto waiting = std::chrono::steady_clock::now();
    // 维护替换时旧守护可能尚在观察正常退出；不要求旧实例替换或强杀守护。
    for (;;)
    {
        const int32_t acquired = CSystem::acquireProcessLease(slot, false, 0, lease);
        if (acquired == 1) { break; }
        if (acquired < 0 || std::chrono::steady_clock::now() - waiting >= std::chrono::seconds(2) || !lifetime.poll())
        {
            return 1;
        }
        CSystem::Sleep(10);
    }
    if (!CSystem::setProcessLeaseReady(lease, true))
    {
        return 1;
    }
    std::vector<std::string> options(arguments);
    options.insert(options.begin(), "--supervised");
    uint32_t backoff = 1000;
    auto started = std::chrono::steady_clock::now();
    while (lifetime.poll())
    {
        int32_t code = 0;
        const int32_t state = CSystem::pollProcessMonitor(monitor, code);
        if (state < 0) { return 1; }
        if (state == 0) { CSystem::Sleep(20); continue; }
        if (code == 0 || code == 2) { return 0; }
        if (std::chrono::steady_clock::now() - started >= std::chrono::seconds(60))
        {
            backoff = 1000;
        }
        CSystem::closeProcessMonitor(monitor);
        for (;;)
        {
            const auto retry = std::chrono::steady_clock::now() + std::chrono::milliseconds(backoff);
            while (std::chrono::steady_clock::now() < retry)
            {
                if (!lifetime.poll()) { return 0; }
                CSystem::Sleep(20);
            }
            // 正常会话结束的广播也可能在退避边界到达，启动前再次分派。
            if (!lifetime.poll()) { return 0; }
            uint32_t pid = 0;
            const uint32_t error = CSystem::startDetachedProcess(executable, options, pid, monitor);
            backoff = backoff >= 16000 ? 30000 : backoff * 2;
            if (error == 0) { started = std::chrono::steady_clock::now(); break; }
        }
    }
    return lifetime.failed() ? 1 : 0;
#else
    (void)parent;
    (void)executable;
    (void)arguments;
    return 1;
#endif
}