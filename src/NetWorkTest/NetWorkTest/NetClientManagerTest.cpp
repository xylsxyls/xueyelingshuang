#include "NetClientManagerTest.h"
#include "NetManagerTestSystem.h"
#include "NetIsolatedTest.h"
#include "NetClientOpaqueGatewayTestAccess.h"
#include <stdexcept>
#include "../../NetClientManager/NetClientManager/NetClientManager.h"
#include "../../NetClientManager/NetClientManager/NetClientGatewayTask.h"
#include "../../NetClientManager/NetClientManager/NetClientGatewayLifetime.h"
#include "../../NetClientManager/NetClientManager/NetClientGatewayWatchdog.h"
#include "NetWork/NetWorkAPI.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <csignal>
#include <cstdio>
#include <cstring>

// 仅测试翻译单元隔离租约和守护，管理类方法及其控制流程取自生产源码。
#define CSystem NetManagerTestSystem
#define NetClientGatewayWatchdog NetManagerTestWatchdog
#define main NetClientManagerProductionEntryForTest
#include "../../NetClientManager/NetClientManager/NetClientManager.cpp"
#undef main
#undef NetClientGatewayWatchdog
#undef CSystem

#if defined(_WIN32) && defined(_DEBUG)
#include <crtdbg.h>

// 只拒绝当前测试主线程的一次分配，工作线程不参与故障注入。
static DWORD g_managerAllocationThread = 0;
// 剩余主线程分配次数，负数关闭注入。
static int g_managerAllocationBudget = -1;
// 当前注入点是否实际拒绝过分配。
static bool g_managerAllocationRejected = false;
// 测试开始前已有的CRT分配钩子，未拒绝时继续调用。
static _CRT_ALLOC_HOOK g_managerPreviousAllocationHook = nullptr;

/** 仅在选定主线程分配点注入一次失败，释放动作及其他线程保持原行为
@param [in] operation CRT分配操作
@param [in] pointer 原块
@param [in] size 请求字节
@param [in] block CRT块类型
@param [in] request 分配编号
@param [in] filename CRT源文件
@param [in] line CRT源行
@return 是否允许本次CRT操作
*/
static int __cdecl ManagerAllocationHook(int operation, void* pointer, size_t size, int block, long request,
    const unsigned char* filename, int line)
{
    if (operation == _HOOK_ALLOC && GetCurrentThreadId() == g_managerAllocationThread && g_managerAllocationBudget >= 0)
    {
        if (g_managerAllocationBudget-- == 0)
        {
            g_managerAllocationRejected = true;
            return FALSE;
        }
    }
    return g_managerPreviousAllocationHook == nullptr ? TRUE :
        g_managerPreviousAllocationHook(operation, pointer, size, block, request, filename, line);
}
#endif

/** 管理类生命周期单测，覆盖重复调用、失败恢复、停止、信号恢复及分配失败回收
@param [in] argc 原平台参数数量
@param [in] argv 原平台参数，唯一业务参数为独占临时目录
@return 0通过，1检查失败，2参数错误
*/
int NetClientManagerTest::check(const std::string& root)
{
#ifdef _WIN32
    if (!CSystem::bindInheritedOutput())
    {
        return 1;
    }
#endif
    NetClientManager& manager = NetClientManager::instance();
    try
    {
        std::vector<std::string> paths(1, root);
        uint32_t checks = 0;
        // condition为独立预期，message只含用例名；失败打印后交给main统一清理。
        const auto check = [&checks](bool condition, const char* message) -> bool
        {
            ++checks;
            if (!condition)
            {
                std::fprintf(stderr, "MANAGER_UNIT_FAILED %s\n", message);
            }
            return condition;
        };
        if (!check(&manager == &NetClientManager::instance(), "singleton") ||
            !check(manager.run() == 1, "run-before-init") ||
            !check(manager.uninit() && manager.uninit(), "empty-uninit"))
        {
            return 1;
        }
        std::vector<std::string> help(1, "--help");
        if (!check(manager.init(help) == 0, "help-init") ||
            !check(manager.init(help) == 1, "duplicate-init") ||
            !check(manager.run() == 0, "help-run") ||
            !check(manager.run() == 1, "duplicate-run") ||
            !check(manager.uninit() && manager.uninit(), "help-uninit"))
        {
            return 1;
        }
        std::vector<std::string> invalid(1, "--port=70000");
        if (!check(manager.init(invalid) == 2, "invalid-config") ||
            !check(manager.run() == 1, "failed-init-not-runnable") ||
            !check(manager.uninit(), "failed-init-uninit"))
        {
            return 1;
        }
        std::vector<std::string> arguments;
        arguments.push_back("--protocol=3");
        arguments.push_back("--address=127.0.0.1");
        arguments.push_back("--port=9");
        arguments.push_back("--run-for-ms=40");
        arguments.push_back("--cache-file=" + CSystem::joinPath(paths[0], "cache.dat"));
        // 先实际运行一轮，使第三方懒初始化与后续任务/句柄回收检查分离。
        if (!check(manager.init(arguments) == 0, "warm-init") ||
            !check(manager.run() == 0, "warm-run") || !check(manager.uninit(), "warm-uninit"))
        {
            return 1;
        }
        const int32_t threads = CTaskThreadManager::instance().count();
        for (uint32_t cycle = 0; cycle < 4; ++cycle)
        {
            const auto originalSignal = std::signal(SIGINT, SIG_IGN);
            if (!check(originalSignal != SIG_ERR, "test-signal-install") ||
                !check(manager.init(arguments) == 0, "cycle-init") ||
                !check(manager.init(arguments) == 1, "live-duplicate-init"))
            {
                return 1;
            }
            if (cycle == 0)
            {
                manager.stop();
                manager.stop();
            }
            else if (cycle == 1)
            {
                std::raise(SIGINT);
            }
            if (cycle != 3 && !check(manager.run() == 0, "cycle-run"))
            {
                return 1;
            }
            if (!check(manager.uninit() && manager.uninit(), "cycle-uninit") ||
                !check(CTaskThreadManager::instance().count() == threads, "thread-reclaimed") ||
                !check(std::signal(SIGINT, SIG_IGN) == SIG_IGN, "signal-restored"))
            {
                return 1;
            }
            std::signal(SIGINT, originalSignal);
        }
        std::vector<std::string> failed(arguments);
        failed[1] = "--address=invalid-ip";
        if (!check(manager.init(failed) == 0, "async-failure-prepare") ||
            !check(manager.run() == 1, "async-failure-reported") ||
            !check(manager.uninit(), "async-failure-uninit") ||
            !check(CTaskThreadManager::instance().count() == threads, "failed-thread-reclaimed"))
        {
            return 1;
        }
        uint32_t rejected = 0;
#if defined(_WIN32) && defined(_DEBUG)
        g_managerAllocationThread = GetCurrentThreadId();
        for (int budget = 0; budget < 160; ++budget)
        {
            g_managerAllocationBudget = budget;
            g_managerAllocationRejected = false;
            g_managerPreviousAllocationHook = _CrtSetAllocHook(ManagerAllocationHook);
            const int initialized = manager.init(arguments);
            _CrtSetAllocHook(g_managerPreviousAllocationHook);
            g_managerAllocationBudget = -1;
            if (g_managerAllocationRejected)
            {
                ++rejected;
            }
            // 返回成功的注入点可能属于可选准备；同样必须可停止且收回任务。
            if (initialized == 0)
            {
                manager.stop();
                manager.run();
            }
            if (!check(manager.uninit() && manager.uninit(), "allocation-uninit") ||
                !check(CTaskThreadManager::instance().count() == threads, "allocation-thread-reclaimed"))
            {
                return 1;
            }
        }
        if (!check(rejected > 0, "allocation-rejection-exercised"))
        {
            return 1;
        }
        // 直接在网关初始化阶段注入，不能仅检查外层管理任务的准备阶段。
        std::vector<NetClientGatewayAddress> seeds(1);
        seeds[0].m_address = "127.0.0.1";
        seeds[0].m_port = 9;
        const std::string emptyCache;
        uint32_t opaqueFailures = 0;
        for (int budget = 0; budget < 120; ++budget)
        {
            NetClientOpaqueGateway gateway;
            std::printf("OPAQUE_INIT_FAULT budget=%d\n", budget);
            g_managerAllocationBudget = budget;
            g_managerAllocationRejected = false;
            g_managerPreviousAllocationHook = _CrtSetAllocHook(ManagerAllocationHook);
            const int32_t initialized = gateway.init(seeds, emptyCache);
            _CrtSetAllocHook(g_managerPreviousAllocationHook);
            g_managerAllocationBudget = -1;
            if (initialized != NI_OK)
            {
                ++opaqueFailures;
                NetGatewayDiscovery& discovery = gateway.*netOpaqueTestMember(NetOpaqueTestDiscovery());
                if (!check(discovery.stopped(), "opaque-init-failure-stops-discovery"))
                {
                    gateway.shutdown();
                    return 1;
                }
            }
            gateway.shutdown();
            if (!check(ProcessWork::instance().receiveBufferCount() == 0 &&
                CTaskThreadManager::instance().count() == threads, "opaque-init-resources-reclaimed"))
            {
                return 1;
            }
        }
        if (!check(opaqueFailures != 0, "opaque-init-failure-exercised"))
        {
            return 1;
        }
        std::printf("OPAQUE_INIT_ROLLBACK attempts=120 failed_initializations=%u PASS\n", opaqueFailures);
#endif
        if (!check(manager.init(arguments) == 0 && manager.run() == 0 && manager.uninit(), "final-recovery"))
        {
            return 1;
        }
        std::printf("MANAGER_UNIT_PASS checks=%u rejected=%u\n", checks, rejected);
        return 0;
    }
    catch (const std::exception& error)
    {
#if defined(_WIN32) && defined(_DEBUG)
        _CrtSetAllocHook(g_managerPreviousAllocationHook);
        g_managerAllocationBudget = -1;
#endif
        manager.uninit();
        std::fprintf(stderr, "MANAGER_UNIT_FAILED unexpected-exception %s\n", error.what());
        return 1;
    }
    catch (...)
    {
#if defined(_WIN32) && defined(_DEBUG)
        _CrtSetAllocHook(g_managerPreviousAllocationHook);
        g_managerAllocationBudget = -1;
#endif
        manager.uninit();
        std::fputs("MANAGER_UNIT_FAILED unknown-exception\n", stderr);
        return 1;
    }
}


void NetClientManagerTest::run()
{
    if (NetIsolatedTest::run("--isolated-manager=client", 60000) != 0)
    {
        throw std::runtime_error("NetClientManagerTest failed");
    }
}

int NetClientManagerTest::host(int argc, char** argv)
{
    return NetClientManagerProductionEntryForTest(argc, argv);
}