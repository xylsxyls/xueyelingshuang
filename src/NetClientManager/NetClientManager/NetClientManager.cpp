#include "NetClientManager.h"
#include "NetClientGatewayTask.h"
#include "NetClientGatewayLifetime.h"
#include "NetClientGatewayWatchdog.h"
// 原生文件后端由NetWork正式宿主入口配置
#include "NetWork/NetWorkHostAPI.h"
#include "LibuvTcp/LibuvHostAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <csignal>
#include <cstdio>
#include <cstring>

NetClientGatewayConfiguration::NetClientGatewayConfiguration() :
m_port(5203),
m_runForMs(0),
m_protocol(3)
{
    std::memset(m_device, 0, sizeof(m_device));
}

// 标准信号处理器与主循环共享的退出位，只做信号安全的赋值
static volatile sig_atomic_t g_clientGatewaySignal = 0;
#ifdef _WIN32
// Windows控制台回调线程通知主循环停止，不在系统回调内清理工作线程
static std::atomic<bool> g_clientGatewayConsole(false);

/** 系统回调只置停止位
@param [in] event 控制台事件
@return 是否接纳
*/
static BOOL WINAPI ClientGatewayConsole(DWORD event)
{
    if (event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT || event == CTRL_CLOSE_EVENT || event == CTRL_SHUTDOWN_EVENT ||
        event == CTRL_LOGOFF_EVENT)
    {
        g_clientGatewayConsole.store(true);
        return TRUE;
    }
    return FALSE;
}

#endif

/** 标准信号只置位
@param [in] signal 标准信号
*/
static void ClientGatewaySignal(int signal)
{
    (void)signal;
    g_clientGatewaySignal = 1;
}

/** 有界十进制解析
@param [in] text 参数
@param [in] maximum 上限
@param [out] value 成功数值
@return 是否成功
*/
static bool ClientGatewayNumber(const std::string& text, uint32_t maximum, uint32_t& value)
{
    if (text.empty() || text.size() > 10 || text.find_first_not_of("0123456789") != std::string::npos)
    {
        return false;
    }
    uint64_t number = 0;
    for (char byte : text)
    {
        number = number * 10 + byte - '0';
    }
    if (number > maximum)
    {
        return false;
    }
    value = static_cast<uint32_t>(number);
    return true;
}

/** 复用系统库有界读取，不复制平台文件接口
@param [in] path UTF8文件
@param [in] maximum 字节上限
@param [out] output 文本/PEM
@return 非空文件读取成功
*/
static bool ReadClientGatewayFile(const std::string& path, uint32_t maximum, std::string& output)
{
    std::vector<uint8_t> bytes;
    if (!CSystem::readBinaryFile(CStringManager::Utf8ToUnicode(path), maximum, bytes) || bytes.empty())
    {
        return false;
    }
    output.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    netInterfaceWipe(bytes.data(), static_cast<uint32_t>(bytes.size()));
    return true;
}

/** 持久设备号从配置读取，每次连接不重新生成
@param [in] path 32个十六进制字符文件
@param [out] device 16字节设备号
@return 非零有效设备号
*/
static bool ReadClientGatewayDevice(const std::string& path, uint8_t* device)
{
    std::string text;
    if (!ReadClientGatewayFile(path, 34, text))
    {
        return false;
    }
    while (!text.empty() && (text.back() == '\r' || text.back() == '\n'))
    {
        text.pop_back();
    }
    if (text.size() != 32)
    {
        return false;
    }
    uint8_t combined = 0;
    for (uint32_t index = 0; index < 32; ++index)
    {
        const char byte = text[index];
        const uint32_t nibble = byte >= '0' && byte <= '9'   ? byte - '0'
                                : byte >= 'a' && byte <= 'f' ? byte - 'a' + 10
                                : byte >= 'A' && byte <= 'F' ? byte - 'A' + 10
                                                             : 16;
        if (nibble > 15)
        {
            return false;
        }
        device[index / 2] = static_cast<uint8_t>((index % 2 == 0 ? 0 : device[index / 2] << 4) | nibble);
        combined |= device[index / 2];
    }
    return combined != 0;
}

/** 严格新网关CLI，不隐式读取生产地址或明文配置
@param [in] arguments UTF8参数
@param [out] configuration 启动配置
@param [out] stopFile 本次停止文件
@return 是否完整合法
*/
static bool ParseClientGateway(const std::vector<std::string>& arguments, NetClientGatewayConfiguration& configuration,
                               std::string& stopFile)
{
    std::map<std::string, std::string> options;
    for (const auto& argument : arguments)
    {
        const size_t separator = argument.find('=');
        const std::string key = argument.substr(0, separator);
        if (separator == std::string::npos || separator + 1 == argument.size() || options.count(key) != 0 ||
            (key != "--address" && key != "--port" && key != "--hostname" && key != "--authority" && key != "--certificate" &&
             key != "--private-key" && key != "--device-file" && key != "--run-for-ms" && key != "--stop-file" && key != "--protocol" && key != "--cache-file" && key != "--seeds"))
        {
            return false;
        }
        options[key] = argument.substr(separator + 1);
    }
    if (options.count("--protocol") != 0 &&
        (!ClientGatewayNumber(options["--protocol"], 3, configuration.m_protocol) || configuration.m_protocol < 2))
    {
        return false;
    }
    if (options.count("--address") == 0 && configuration.m_protocol != 3) { return false; }
    const bool transparent = configuration.m_protocol == 3;
    if (transparent)
    {
        configuration.m_cachePath = options.count("--cache-file") ? options["--cache-file"] :
            CSystem::joinPath(CSystem::GetName(CSystem::currentExecutablePathUtf8(), 4), "gateway/nodes.cache");
        if (options.count("--seeds") && NetGatewayDiscovery::parseSeeds(options["--seeds"], configuration.m_seeds) != NI_OK)
        {
            return false;
        }
    }
    else if (options.count("--cache-file") || options.count("--seeds")) { return false; }
    if (transparent ? (options.count("--hostname") != 0 || options.count("--authority") != 0 ||
                          options.count("--certificate") != 0 || options.count("--private-key") != 0 || options.count("--device-file") != 0) :
                      (options.count("--hostname") == 0 || options.count("--authority") == 0 ||
                          options.count("--certificate") == 0 || options.count("--private-key") == 0 || options.count("--device-file") == 0))
    {
        return false;
    }
    configuration.m_address = options.count("--address") ? options["--address"] : "175.24.198.162";
    if (!transparent) { configuration.m_hostname = options["--hostname"]; }
    if (options.count("--port") != 0 && (!ClientGatewayNumber(options["--port"], 65535, configuration.m_port) || configuration.m_port == 0))
    {
        return false;
    }
    if (options.count("--run-for-ms") != 0 && !ClientGatewayNumber(options["--run-for-ms"], 86400000, configuration.m_runForMs))
    {
        return false;
    }
    if (!transparent && (!ReadClientGatewayFile(options["--authority"], 131071, configuration.m_authority) ||
        !ReadClientGatewayFile(options["--certificate"], 131071, configuration.m_certificate) ||
        !ReadClientGatewayFile(options["--private-key"], 32767, configuration.m_privateKey) ||
        !ReadClientGatewayDevice(options["--device-file"], configuration.m_device)))
    {
        return false;
    }
    if (options.count("--stop-file") != 0)
    {
        stopFile = options["--stop-file"];
        if (CSystem::fileExists(CStringManager::Utf8ToUnicode(stopFile)))
        {
            return false;
        }
    }
    return true;
}

/** 本程序入口将LibuvTcp接入通用宿主表；NetWork不选择具体实现
@return NI_OK或错误码
*/
static int32_t ConfigureGatewayHost()
{
    NetHostFunctions functions = {};
    functions.structSize = sizeof(functions);
    functions.version = 1;
    functions.create = libuvHostCreate;
    functions.listen = libuvHostListen;
    functions.connect = libuvHostConnect;
    functions.poll = libuvHostPoll;
    functions.write = libuvHostWrite;
    functions.pauseRead = libuvHostPauseRead;
    functions.close = libuvHostClose;
    functions.shutdown = libuvHostShutdown;
    functions.destroy = libuvHostDestroy;
    functions.localPort = libuvHostLocalPort;
    functions.peerCount = libuvHostPeerCount;
    functions.entropy = CSystem::secureRandomBytesCallback;
    return netWorkConfigureHost(&functions);
}

NetClientManager::NetClientManager() :
m_stop(false),
m_hostConfigured(false),
m_initialized(false),
m_running(false),
m_completed(false),
m_cleanupFailed(false),
m_thread(0),
m_previousInterrupt(nullptr),
m_previousTerminate(nullptr),
m_interruptInstalled(false),
m_terminateInstalled(false),
m_consoleInstalled(false)
{

}

NetClientManager::~NetClientManager()
{
    uninit();
}

NetClientManager& NetClientManager::instance()
{
    // 先构造任务管理器，保证本管理对象析构兜底时任务管理器仍然存在；此处不创建线程。
    CTaskThreadManager::Instance();
    static NetClientManager s_manager;
    return s_manager;
}

int NetClientManager::init(const std::vector<std::string>& arguments)
{
    if (m_initialized || m_running || m_thread != 0 || m_task || !m_lease.empty() ||
        m_interruptInstalled || m_terminateInstalled || m_consoleInstalled)
    {
        return 1;
    }
    m_stop.store(false);
    m_completed = false;
    m_cleanupFailed = false;
    g_clientGatewaySignal = 0;
#ifdef _WIN32
    g_clientGatewayConsole.store(false);
#endif
    int result = 1;
    try
    {
        result = prepare(arguments);
    }
    catch (...)
    {
        std::fputs("NetClientManager startup failed\n", stderr);
    }
    if (result != 0)
    {
        return uninit() ? result : 1;
    }
    m_initialized = true;
    return 0;
}

int NetClientManager::prepare(const std::vector<std::string>& input)
{
    if (!m_hostConfigured)
    {
        // NetWork同时登记默认文件后端；本对象后续init继续使用同一份进程宿主表。
        if (ConfigureGatewayHost() != NI_OK)
        {
            return 1;
        }
        m_hostConfigured = true;
    }
    std::vector<std::string> arguments(input);
    if (arguments.size() == 1 && arguments[0] == "--help")
    {
        std::puts("Windows background resident: NetClientManager [--background]. Existing instance is reused.");
        std::puts("Maintenance only: --replace. Login startup: --install-autostart / --uninstall-autostart (alone).");
        std::puts("NTW3: NetClientManager --protocol=3 --address=<numeric-IP> [--port=5203] "
                  "[--seeds=<IP:port,[IPv6]:port>] [--cache-file=<UTF8-path>] "
                  "[--run-for-ms=0..86400000] [--stop-file=<fresh-owned-path>]");
        std::puts("NetClientManager --address=<numeric-IP> --hostname=<TLS-DNS> --authority=<CA-PEM> "
                  "--certificate=<local-PEM> --private-key=<local-key> --device-file=<16-byte-hex-file> "
                  "[--port=5203] [--run-for-ms=0..86400000] [--stop-file=<fresh-owned-path>].");
        return 0;
    }
    if (arguments.size() == 1 && (arguments[0] == "--install-autostart" || arguments[0] == "--uninstall-autostart"))
    {
        const std::string command = arguments[0] == "--install-autostart" ?
            "\"" + CSystem::currentExecutablePathUtf8() + "\" --background" : "";
        const uint32_t error = CSystem::setUserLoginStartup("NetClientManager", command);
        if (error != 0) { std::fprintf(stderr, "CLIENT_GATEWAY_AUTOSTART_FAILED error=%u\n", error); return 1; }
        std::puts(command.empty() ? "CLIENT_GATEWAY_AUTOSTART_REMOVED" : "CLIENT_GATEWAY_AUTOSTART_INSTALLED");
        return 0;
    }
    bool replace = false;
    bool background = false;
    bool supervised = false;
    std::vector<std::string> configurationArguments;
    for (const std::string& argument : arguments)
    {
        if (argument == "--background" && !background) { background = true; }
        else if (argument == "--replace" && !replace) { replace = true; }
        else if (argument == "--supervised" && !supervised) { supervised = true; }
        else { configurationArguments.push_back(argument); }
    }
    arguments.swap(configurationArguments);
    // 旧SDK只枚举网关名；崩溃退避时再次触发的启动由网关入口复用独立守护。
    // 内部守护参数只属于Guard入口，正式网关不能再以自身名字常驻为守护。
    for (const std::string& argument : arguments)
    {
        // 先拒绝已移除的入口，不能因复用或--replace影响现有网关的租约。
        if (argument == "--legacy" || argument.compare(0, 9, "--legacy=") == 0)
        {
            std::fputs("Legacy gateway mode has been removed; use --protocol=3\n", stderr);
            return 2;
        }
        if (argument.compare(0, 15, "--watch-parent=") == 0)
        {
            return 2;
        }
    }
#ifndef NET_GATEWAY_TEST_INSTANCE
    if (!replace && !supervised && NetClientGatewayWatchdog::waitForRecovery())
    {
        std::fputs("NetClientManager_REUSED: existing supervisor retains gateway configuration\n", stderr);
        return 0;
    }
#endif
    std::string isolated;
#ifdef NET_GATEWAY_TEST_INSTANCE
    // 只用于独立命名的NetWorkTest宿主；正式两个网关从不编译此宏。
    isolated = std::to_string(CSystem::currentProcessPid());
#endif
    const int32_t acquired = CSystem::acquireProcessLease("NetClientManager", replace, 10000, m_lease, isolated);
    if (acquired != 1)
    {
        std::fputs(acquired == 0 ? "NetClientManager_REUSED: existing instance retained; arguments not applied\n" :
            "NetClientManager lease unavailable or previous instance did not stop in 10 seconds\n", stderr);
        return acquired == 0 ? 0 : 1;
    }
#ifndef NET_GATEWAY_TEST_INSTANCE
    if (!replace && !supervised && NetClientGatewayWatchdog::waitForRecovery())
    {
        std::fputs("NetClientManager_REUSED: existing supervisor retains gateway configuration\n", stderr);
        return 0;
    }
#endif
    if (arguments.size() == 1 && arguments[0].compare(0, 2, "--") != 0)
    {
        arguments[0] = "--address=" + arguments[0];
    }
    NetClientGatewayConfiguration configuration;
    if (!ParseClientGateway(arguments, configuration, m_stopFile))
    {
        std::fputs("invalid client gateway configuration; use --help\n", stderr);
        return 2;
    }
    m_lifetime.reset(new NetClientGatewayLifetime());
    // 回调只在当前主线程分派，m_lifetime销毁前本管理对象始终有效。
    if (!m_lifetime->init([this]() { stopTask(); }))
    {
        return 1;
    }
    // 守护成功持有本进程句柄之后才启动工作并发布ready，业务不拥有守护生命周期。
#ifndef NET_GATEWAY_TEST_INSTANCE
    if (!supervised && !NetClientGatewayWatchdog::start(arguments))
    {
        return 1;
    }

#endif
    m_task.reset(new NetClientGatewayTask(configuration));
    netInterfaceWipe(configuration.m_privateKey.empty() ? nullptr : reinterpret_cast<uint8_t*>(&configuration.m_privateKey[0]),
                     static_cast<uint32_t>(configuration.m_privateKey.size()));
    if (!installSignals())
    {
        return 1;
    }
    m_thread = CTaskThreadManager::Instance().Init();
    if (m_thread == 0)
    {
        return 1;
    }
    auto executor = CTaskThreadManager::Instance().GetThreadInterface(m_thread);
    if (!executor)
    {
        return 1;
    }
    executor->PostTask(m_task);
    executor.reset();
    return 0;
}

bool NetClientManager::installSignals()
{
    m_previousInterrupt = std::signal(SIGINT, ClientGatewaySignal);
    if (m_previousInterrupt == SIG_ERR)
    {
        return false;
    }
    m_interruptInstalled = true;
    m_previousTerminate = std::signal(SIGTERM, ClientGatewaySignal);
    if (m_previousTerminate == SIG_ERR)
    {
        return false;
    }
    m_terminateInstalled = true;
#ifdef _WIN32
    if (!SetConsoleCtrlHandler(ClientGatewayConsole, TRUE))
    {
        return false;
    }
    m_consoleInstalled = true;
#endif
    return true;
}

void NetClientManager::stop()
{
    m_stop.store(true);
}

bool NetClientManager::stopTask()
{
    stop();
    try
    {
        if (!m_lease.empty())
        {
            CSystem::setProcessLeaseReady(m_lease, false);
        }
        if (m_task)
        {
            m_task->StopTask();
        }
        if (m_thread != 0)
        {
            CTaskThreadManager::Instance().WaitForEnd(m_thread);
            m_thread = 0;
        }
        return true;
    }
    catch (...)
    {
        m_cleanupFailed = true;
        std::fputs("NetClientManager cannot finish task cleanup\n", stderr);
        return false;
    }
}

int NetClientManager::run()
{
    if (!m_initialized || m_running || m_completed)
    {
        return 1;
    }
    if (!m_task)
    {
        m_completed = true;
        return 0;
    }
    m_running = true;
    int result = 1;
    try
    {
        bool announced = false;
        while (!m_task->done())
        {
            if (!m_lifetime->poll())
            {
                break;
            }
            if (!announced && m_task->ready() && !m_stop.load() && !CSystem::processLeaseStopRequested(m_lease))
            {
                if (!CSystem::setProcessLeaseReady(m_lease, true))
                {
                    stop();
                    m_task->StopTask();
                    break;
                }
                std::printf("CLIENT_GATEWAY_READY pid=%u\n", static_cast<uint32_t>(CSystem::currentProcessPid()));
                std::fflush(stdout);
                announced = true;
            }
            bool stopRequested = m_stop.load() || g_clientGatewaySignal != 0 || CSystem::processLeaseStopRequested(m_lease);
#ifdef _WIN32
            stopRequested = stopRequested || g_clientGatewayConsole.load();
#endif
            if (!m_stopFile.empty())
            {
                stopRequested = stopRequested || CSystem::fileExists(CStringManager::Utf8ToUnicode(m_stopFile));
            }
            if (stopRequested)
            {
                CSystem::setProcessLeaseReady(m_lease, false);
                stop();
                m_task->StopTask();
            }
            CSystem::Sleep(10);
        }
        if (stopTask() && !m_cleanupFailed && m_task->error() == 0 && !m_lifetime->failed())
        {
            std::puts("CLIENT_GATEWAY_STOPPED");
            result = 0;
        }
        else
        {
            std::fprintf(stderr, "NetClientManager stopped with error %d\n", m_task->error());
        }
    }
    catch (...)
    {
        std::fputs("NetClientManager run failed\n", stderr);
        stopTask();
    }
    m_running = false;
    m_completed = true;
    return result;
}

bool NetClientManager::uninit()
{
    if (m_running)
    {
        stop();
        return false;
    }
    if (!stopTask())
    {
        return false;
    }
    m_task.reset();
    m_lifetime.reset();
    m_stopFile.clear();
    bool restored = true;
#ifdef _WIN32
    if (m_consoleInstalled)
    {
        if (SetConsoleCtrlHandler(ClientGatewayConsole, FALSE))
        {
            m_consoleInstalled = false;
        }
        else
        {
            restored = false;
        }
    }
#endif
    if (m_interruptInstalled)
    {
        if (std::signal(SIGINT, m_previousInterrupt) != SIG_ERR)
        {
            m_interruptInstalled = false;
        }
        else
        {
            restored = false;
        }
    }
    if (m_terminateInstalled)
    {
        if (std::signal(SIGTERM, m_previousTerminate) != SIG_ERR)
        {
            m_terminateInstalled = false;
        }
        else
        {
            restored = false;
        }
    }
    CSystem::releaseProcessLease(m_lease);
    m_initialized = false;
    m_completed = false;
    return restored;
}

/** 进程入口，只读取平台参数并顺序调用管理器的生命周期
@param [in] argc 原平台参数数量
@param [in] argv 原平台参数，Windows经CSystem读取Unicode命令行
@return 正常退出或复用0，启动运行失败1，参数错误2
*/
int main(int argc, char** argv)
{
#ifdef _WIN32
    if (!CSystem::bindInheritedOutput())
    {
        return 1;
    }
#endif
    try
    {
        std::vector<std::string> arguments;
        if (!CSystem::commandArguments(argc, argv, arguments))
        {
            return 2;
        }
        NetClientManager& manager = NetClientManager::instance();
        int result = manager.init(arguments);
        if (result == 0)
        {
            result = manager.run();
        }
        return manager.uninit() ? result : 1;
    }
    catch (...)
    {
        std::fputs("NetClientManager entry failed\n", stderr);
        return 1;
    }
}

#if 0 // MANAGER_LIFECYCLE_UNIT_BEGIN
// 测试时在隔离副本停用上方生产main、启用此main，并定义NET_GATEWAY_TEST_INSTANCE；参数为独占临时目录。
// 测试结束保持此块关闭，不运行生产单例或实际公网端点。
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
int main(int argc, char** argv)
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
        std::vector<std::string> paths;
        if (!CSystem::commandArguments(argc, argv, paths) || paths.size() != 1)
        {
            return 2;
        }
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
        const int32_t threads = CTaskThreadManager::Instance().Count();
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
                !check(CTaskThreadManager::Instance().Count() == threads, "thread-reclaimed") ||
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
            !check(CTaskThreadManager::Instance().Count() == threads, "failed-thread-reclaimed"))
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
                !check(CTaskThreadManager::Instance().Count() == threads, "allocation-thread-reclaimed"))
            {
                return 1;
            }
        }
        if (!check(rejected > 0, "allocation-rejection-exercised"))
        {
            return 1;
        }
#endif
        if (!check(manager.init(arguments) == 0 && manager.run() == 0 && manager.uninit(), "final-recovery"))
        {
            return 1;
        }
        std::printf("MANAGER_UNIT_PASS checks=%u rejected=%u\n", checks, rejected);
        return 0;
    }
    catch (...)
    {
#if defined(_WIN32) && defined(_DEBUG)
        _CrtSetAllocHook(g_managerPreviousAllocationHook);
        g_managerAllocationBudget = -1;
#endif
        manager.uninit();
        std::fputs("MANAGER_UNIT_FAILED unexpected-exception\n", stderr);
        return 1;
    }
}
#endif // MANAGER_LIFECYCLE_UNIT_END